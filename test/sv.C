#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "TCanvas.h"
#include "TChain.h"
#include "TH1D.h"
#include "TLorentzVector.h"
#include "TLegend.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"

namespace {

const char *kDataDataset = "/REPLACE_WITH_DATA_USER_DATASET/USER";
const char *kMcDataset = "/REPLACE_WITH_MC_USER_DATASET/USER";
const char *kDbsInstance = "prod/phys03";
const char *kRedirector = "root://cms-xrd-global.cern.ch/";

const double kMuonAbsEtaMax = 1.4;
const double kMuonPtMin = 10.0;
const double kDimuonPtMin = 20.0;
const double kDimuonPtMax = 200.0;
const double kDimuonAbsYMax = 1.2;
const double kVProbMin = 0.01;
const int kNBins = 60;

struct Branches {
  UInt_t nonia = 0;
  UInt_t trigger = 0;
  Int_t charge = 0;
  Float_t vProb = -1.f;
  Float_t dimuon_sv_x = 0.f;
  Float_t dimuon_sv_y = 0.f;
  Float_t dimuon_sv_z = 0.f;
  TLorentzVector *dimuon_p4 = nullptr;
  TLorentzVector *muonP_p4 = nullptr;
  TLorentzVector *muonM_p4 = nullptr;
  TLorentzVector *gen_muonP_p4 = nullptr;
  TLorentzVector *gen_muonM_p4 = nullptr;
};

struct SelectedValues {
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> z;
};

bool IsPlaceholderDataset(const std::string &dataset) {
  return dataset.find("REPLACE_WITH") != std::string::npos;
}

std::vector<std::string> SplitLines(const TString &text) {
  std::vector<std::string> lines;
  std::stringstream stream(text.Data());
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

std::vector<std::string> QueryDatasetFiles(const std::string &dataset) {
  const TString command = Form(
      "dasgoclient -query=\"file dataset=%s instance=%s\" 2>/dev/null",
      dataset.c_str(), kDbsInstance);
  const TString output = gSystem->GetFromPipe(command);

  if (output.IsNull()) {
    std::cerr << "No files returned for dataset " << dataset
              << ". Check the dataset name and DBS instance." << std::endl;
  }

  return SplitLines(output);
}

void AddDatasetFiles(TChain &chain, const std::string &dataset) {
  const std::vector<std::string> files = QueryDatasetFiles(dataset);
  for (const std::string &file : files) {
    if (file.rfind("/store/", 0) == 0) {
      chain.Add((std::string(kRedirector) + file).c_str());
    } else {
      chain.Add(file.c_str());
    }
  }
}

void ConfigureBranches(TChain &chain, Branches &branches, const bool isMC) {
  chain.SetBranchStatus("*", 0);
  chain.SetBranchStatus("nonia", 1);
  chain.SetBranchStatus("trigger", 1);
  chain.SetBranchStatus("charge", 1);
  chain.SetBranchStatus("vProb", 1);
  chain.SetBranchStatus("dimuon_sv_x", 1);
  chain.SetBranchStatus("dimuon_sv_y", 1);
  chain.SetBranchStatus("dimuon_sv_z", 1);
  chain.SetBranchStatus("dimuon_p4*", 1);
  chain.SetBranchStatus("muonP_p4*", 1);
  chain.SetBranchStatus("muonM_p4*", 1);

  chain.SetBranchAddress("nonia", &branches.nonia);
  chain.SetBranchAddress("trigger", &branches.trigger);
  chain.SetBranchAddress("charge", &branches.charge);
  chain.SetBranchAddress("vProb", &branches.vProb);
  chain.SetBranchAddress("dimuon_sv_x", &branches.dimuon_sv_x);
  chain.SetBranchAddress("dimuon_sv_y", &branches.dimuon_sv_y);
  chain.SetBranchAddress("dimuon_sv_z", &branches.dimuon_sv_z);
  chain.SetBranchAddress("dimuon_p4", &branches.dimuon_p4);
  chain.SetBranchAddress("muonP_p4", &branches.muonP_p4);
  chain.SetBranchAddress("muonM_p4", &branches.muonM_p4);

  if (isMC) {
    chain.SetBranchStatus("gen_muonP_p4*", 1);
    chain.SetBranchStatus("gen_muonM_p4*", 1);
    chain.SetBranchAddress("gen_muonP_p4", &branches.gen_muonP_p4);
    chain.SetBranchAddress("gen_muonM_p4", &branches.gen_muonM_p4);
  }
}

bool PassCommonSelection(const Branches &branches) {
  if (!branches.dimuon_p4 || !branches.muonP_p4 || !branches.muonM_p4) {
    return false;
  }
  if (std::abs(branches.muonP_p4->Eta()) > kMuonAbsEtaMax) {
    return false;
  }
  if (std::abs(branches.muonM_p4->Eta()) > kMuonAbsEtaMax) {
    return false;
  }
  if (branches.muonP_p4->Pt() <= kMuonPtMin) {
    return false;
  }
  if (branches.muonM_p4->Pt() <= kMuonPtMin) {
    return false;
  }
  if (branches.nonia != 1 || !branches.trigger || branches.charge != 0 ||
      branches.vProb <= kVProbMin) {
    return false;
  }
  if (branches.dimuon_p4->Pt() <= kDimuonPtMin ||
      branches.dimuon_p4->Pt() >= kDimuonPtMax) {
    return false;
  }
  if (std::abs(branches.dimuon_p4->Rapidity()) >= kDimuonAbsYMax) {
    return false;
  }
  return true;
}

bool PassMcSelection(const Branches &branches) {
  if (!branches.gen_muonP_p4 || !branches.gen_muonM_p4) {
    return false;
  }
  if (std::abs(branches.gen_muonP_p4->Eta()) > kMuonAbsEtaMax) {
    return false;
  }
  if (std::abs(branches.gen_muonM_p4->Eta()) > kMuonAbsEtaMax) {
    return false;
  }
  if (branches.gen_muonP_p4->Pt() <= kMuonPtMin) {
    return false;
  }
  if (branches.gen_muonM_p4->Pt() <= kMuonPtMin) {
    return false;
  }
  return true;
}

SelectedValues CollectSelectedValues(TChain &chain, const bool isMC) {
  Branches branches;
  ConfigureBranches(chain, branches, isMC);

  SelectedValues values;
  const Long64_t nEntries = chain.GetEntries();
  for (Long64_t entry = 0; entry < nEntries; ++entry) {
    chain.GetEntry(entry);
    if (!PassCommonSelection(branches)) {
      continue;
    }
    if (isMC && !PassMcSelection(branches)) {
      continue;
    }

    values.x.push_back(branches.dimuon_sv_x);
    values.y.push_back(branches.dimuon_sv_y);
    values.z.push_back(branches.dimuon_sv_z);
  }
  return values;
}

std::pair<double, double> ComputeRange(const std::vector<double> &dataValues,
                                       const std::vector<double> &mcValues) {
  double minValue = std::numeric_limits<double>::max();
  double maxValue = std::numeric_limits<double>::lowest();

  const auto updateRange = [&](const std::vector<double> &values) {
    for (const double value : values) {
      minValue = std::min(minValue, value);
      maxValue = std::max(maxValue, value);
    }
  };

  updateRange(dataValues);
  updateRange(mcValues);

  if (minValue > maxValue) {
    return std::make_pair(-1.0, 1.0);
  }

  if (minValue == maxValue) {
    const double padding = std::max(1e-3, std::abs(minValue) * 0.1);
    return std::make_pair(minValue - padding, maxValue + padding);
  }

  const double padding = 0.1 * (maxValue - minValue);
  return std::make_pair(minValue - padding, maxValue + padding);
}

void FillHistogram(TH1D &hist, const std::vector<double> &values) {
  for (const double value : values) {
    hist.Fill(value);
  }
}

void NormalizeHistogram(TH1D &hist) {
  const double integral = hist.Integral();
  if (integral > 0.) {
    hist.Scale(1. / integral);
  }
}

void DrawComparison(const char *axisName, const char *xTitle,
                    const std::vector<double> &dataValues,
                    const std::vector<double> &mcValues) {
  const std::pair<double, double> range = ComputeRange(dataValues, mcValues);
  TH1D hData(Form("hData_%s", axisName), "", kNBins, range.first, range.second);
  TH1D hMc(Form("hMc_%s", axisName), "", kNBins, range.first, range.second);

  FillHistogram(hData, dataValues);
  FillHistogram(hMc, mcValues);
  NormalizeHistogram(hData);
  NormalizeHistogram(hMc);

  hData.SetLineColor(kBlack);
  hData.SetMarkerColor(kBlack);
  hData.SetMarkerStyle(20);
  hData.SetMarkerSize(0.9);
  hData.SetLineWidth(2);
  hData.GetXaxis()->SetTitle(xTitle);
  hData.GetYaxis()->SetTitle("A.U.");

  hMc.SetLineColor(kRed + 1);
  hMc.SetLineWidth(2);

  const double maxY = std::max(hData.GetMaximum(), hMc.GetMaximum());
  hData.SetMaximum(maxY > 0. ? 1.2 * maxY : 1.0);

  TCanvas canvas(Form("c_%s", axisName), "", 800, 600);
  canvas.SetMargin(0.12, 0.04, 0.12, 0.04);
  hData.Draw("E1");
  hMc.Draw("HIST SAME");
  hData.Draw("E1 SAME");

  TLegend legend(0.68, 0.78, 0.88, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(&hData, "Data", "lep");
  legend.AddEntry(&hMc, "MC", "l");
  legend.Draw();

  canvas.SaveAs(Form("sv_%s.pdf", axisName));
}

}  // namespace

void sv() {
  gStyle->SetOptStat(0);

  const std::string dataDataset = kDataDataset;
  const std::string mcDataset = kMcDataset;
  if (IsPlaceholderDataset(dataDataset) || IsPlaceholderDataset(mcDataset)) {
    std::cout << "Please replace kDataDataset and kMcDataset in test/sv.C "
                 "with the published USER datasets before running."
              << std::endl;
    return;
  }

  TChain dataChain("mm_tree");
  TChain mcChain("mm_tree");
  AddDatasetFiles(dataChain, dataDataset);
  AddDatasetFiles(mcChain, mcDataset);

  if (dataChain.GetNtrees() == 0 || mcChain.GetNtrees() == 0) {
    std::cerr << "Failed to attach files for one or both datasets." << std::endl;
    return;
  }

  const SelectedValues dataValues = CollectSelectedValues(dataChain, false);
  const SelectedValues mcValues = CollectSelectedValues(mcChain, true);

  std::cout << "Selected entries: data=" << dataValues.x.size()
            << ", mc=" << mcValues.x.size() << std::endl;

  DrawComparison("x", "x^{#mu#mu} [cm]", dataValues.x, mcValues.x);
  DrawComparison("y", "y^{#mu#mu} [cm]", dataValues.y, mcValues.y);
  DrawComparison("z", "z^{#mu#mu} [cm]", dataValues.z, mcValues.z);
}
