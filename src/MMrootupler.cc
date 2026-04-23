// Description: Dump  Onia(mu+ mu-)  decays
// Author:  Alberto Sanchez Hernandez

#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "DataFormats/PatCandidates/interface/CompositeCandidate.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/HepMCCandidate/interface/GenParticleFwd.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/Common/interface/TriggerNames.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "TLorentzVector.h"
#include "TTree.h"

class MMrootupler : public edm::stream::EDAnalyzer<>
{
public:
   explicit MMrootupler(const edm::ParameterSet &);
   static void fillDescriptions(edm::ConfigurationDescriptions &descriptions);

private:
   UInt_t getTriggerBits(const edm::Event &);
   bool isAncestor(const reco::Candidate *, const reco::Candidate *);
   const reco::Candidate *GetAncestor(const reco::Candidate *);

   void analyze(const edm::Event &, const edm::EventSetup &) override;

   std::string file_name;
   edm::EDGetTokenT<pat::CompositeCandidateCollection> dimuons_;
   edm::EDGetTokenT<reco::VertexCollection> primaryvertices_;
   edm::EDGetTokenT<edm::TriggerResults> triggerresults_;

   int pdgid_;
   std::vector<double> OniaMassCuts_;
   std::vector<std::string> FilterNames_;
   bool isMC_;
   bool only_best_;
   bool only_gen_;

   UInt_t run;
   ULong64_t event;
   UInt_t lumiblock;
   UInt_t nonia;
   UInt_t trigger;
   Int_t charge;

   TLorentzVector dimuon_p4;
   TLorentzVector muonP_p4;
   TLorentzVector muonM_p4;

   Float_t vProb;
   Float_t dimuon_sv_x;
   Float_t dimuon_sv_y;
   Float_t dimuon_sv_z;
   Float_t DCA;
   Float_t ppdlPV;
   Float_t ppdlErrPV;
   Float_t cosAlpha;
   Float_t ppdlBS;
   Float_t ppdlErrBS;
   Float_t cosAlphaBS;

   UInt_t numPrimaryVertices;

   TTree *mm_tree;

   Int_t mother_pdgId;
   Int_t dimuon_pdgId;
   TLorentzVector gen_dimuon_p4;
   TLorentzVector gen_muonP_p4;
   TLorentzVector gen_muonM_p4;

   edm::EDGetTokenT<reco::GenParticleCollection> genCands_;
   edm::EDGetTokenT<pat::PackedGenParticleCollection> packCands_;
};

MMrootupler::MMrootupler(const edm::ParameterSet &iConfig) : dimuons_(consumes<pat::CompositeCandidateCollection>(iConfig.getParameter<edm::InputTag>("dimuons"))),
                                                             primaryvertices_(consumes<reco::VertexCollection>(iConfig.getParameter<edm::InputTag>("primaryVertices"))),
                                                             triggerresults_(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("TriggerResults"))),
                                                             pdgid_(iConfig.getParameter<uint32_t>("onia_pdgid")),
                                                             OniaMassCuts_(iConfig.getParameter<std::vector<double>>("onia_mass_cuts")),
                                                             FilterNames_(iConfig.getParameter<std::vector<std::string>>("FilterNames")),
                                                             isMC_(iConfig.getParameter<bool>("isMC")),
                                                             only_best_(iConfig.getParameter<bool>("only_best")),
                                                             only_gen_(iConfig.getParameter<bool>("only_gen"))
{
   edm::Service<TFileService> fs;
   mm_tree = fs->make<TTree>("mm_tree", "Tree of dimuons");

   mm_tree->Branch("run", &run, "run/i");
   mm_tree->Branch("event", &event, "event/l");
   mm_tree->Branch("lumiblock", &lumiblock, "lumiblock/i");

   if (!only_gen_)
   { // 已重建
      mm_tree->Branch("nonia", &nonia, "nonia/i");
      mm_tree->Branch("trigger", &trigger, "trigger/i");
      mm_tree->Branch("charge", &charge, "charge/I");

      mm_tree->Branch("dimuon_p4", "TLorentzVector", &dimuon_p4); // 母粒子四动量
      mm_tree->Branch("muonP_p4", "TLorentzVector", &muonP_p4);
      mm_tree->Branch("muonM_p4", "TLorentzVector", &muonM_p4);

      mm_tree->Branch("vProb", &vProb, "vProb/F");
      mm_tree->Branch("dimuon_sv_x", &dimuon_sv_x, "dimuon_sv_x/F");
      mm_tree->Branch("dimuon_sv_y", &dimuon_sv_y, "dimuon_sv_y/F");
      mm_tree->Branch("dimuon_sv_z", &dimuon_sv_z, "dimuon_sv_z/F");
      mm_tree->Branch("DCA", &DCA, "DCA/F");
      mm_tree->Branch("ppdlPV", &ppdlPV, "ppdlPV/F");
      mm_tree->Branch("ppdlErrPV", &ppdlErrPV, "ppdlErrPV/F");
      mm_tree->Branch("cosAlpha", &cosAlpha, "cosAlpha/F");
      mm_tree->Branch("ppdlBS", &ppdlBS, "ppdlBS/F");
      mm_tree->Branch("ppdlErrBS", &ppdlErrBS, "ppdlErrBS/F");
      mm_tree->Branch("cosAlphaBS", &cosAlphaBS, "cosAlphaBS/F");

      mm_tree->Branch("numPrimaryVertices", &numPrimaryVertices, "numPrimaryVertices/i");
   }

   if (isMC_ || only_gen_)
   {
      std::cout << "MMrootupler::MMrootupler: Onia id " << pdgid_ << std::endl;
      mm_tree->Branch("mother_pdgId", &mother_pdgId, "mother_pdgId/I");
      mm_tree->Branch("dimuon_pdgId", &dimuon_pdgId, "dimuon_pdgId/I");
      mm_tree->Branch("gen_dimuon_p4", "TLorentzVector", &gen_dimuon_p4);
      mm_tree->Branch("gen_muonP_p4", "TLorentzVector", &gen_muonP_p4);
      mm_tree->Branch("gen_muonM_p4", "TLorentzVector", &gen_muonM_p4);
   }
   genCands_ = consumes<reco::GenParticleCollection>((edm::InputTag) "prunedGenParticles");
   packCands_ = consumes<pat::PackedGenParticleCollection>((edm::InputTag) "packedGenParticles");
}

const reco::Candidate *MMrootupler::GetAncestor(const reco::Candidate *p)
{
   if (p->numberOfMothers())
   {
      if ((p->mother(0))->pdgId() == p->pdgId())
         return GetAncestor(p->mother(0));
      else
         return p->mother(0);
   }
   return p;
}

// Check recursively if any ancestor of particle is the given one
bool MMrootupler::isAncestor(const reco::Candidate *ancestor, const reco::Candidate *particle)
{
   if (ancestor == particle)
      return true;
   for (size_t i = 0; i < particle->numberOfMothers(); i++)
   {
      if (isAncestor(ancestor, particle->mother(i)))
         return true;
   }
   return false;
}

/* Grab Trigger information. Save it in variable trigger, trigger is an uint between 0 and 256, in binary it is:
   (pass 2)(pass 1)(pass 0)
   ex. 7 = pass 0, 1 and 2
   ex. 6 = pass 1, 2
   ex. 1 = pass 0
   */

UInt_t MMrootupler::getTriggerBits(const edm::Event &iEvent)
{
   UInt_t trigger = 0;
   edm::Handle<edm::TriggerResults> triggerresults;
   iEvent.getByToken(triggerresults_, triggerresults);
   if (triggerresults.isValid())
   {
      const edm::TriggerNames &TheTriggerNames = iEvent.triggerNames(*triggerresults);
      for (unsigned int i = 0; i < FilterNames_.size(); i++)
      {
         bool matched = false;
         for (int version = 1; (version < 99 && (!matched)); version++)
         {
            std::stringstream ss;
            ss << FilterNames_[i] << "_v" << version;
            unsigned int bit = TheTriggerNames.triggerIndex(edm::InputTag(ss.str()).label());
            if (bit < triggerresults->size() && triggerresults->accept(bit) && !triggerresults->error(bit))
               matched = true;
         }
         if (matched)
            trigger += (1 << i);
      }
   }
   else
      std::cout << "MMrootupler::getTriggerBits: *** NO triggerResults found *** " << iEvent.id().run() << "," << iEvent.id().event() << std::endl;
   return trigger;
}

// ------------ method called for each event  ------------
void MMrootupler::analyze(const edm::Event &iEvent, const edm::EventSetup &iSetup)
{

   edm::Handle<pat::CompositeCandidateCollection> dimuons;
   iEvent.getByToken(dimuons_, dimuons);

   edm::Handle<reco::VertexCollection> priVtxs;
   iEvent.getByToken(primaryvertices_, priVtxs);

   run = iEvent.id().run();
   event = iEvent.id().event();
   lumiblock = iEvent.id().luminosityBlock();

   numPrimaryVertices = 0;
   if (priVtxs.isValid())
      numPrimaryVertices = (int)priVtxs->size();
   trigger = getTriggerBits(iEvent);

   dimuon_pdgId = 0;
   mother_pdgId = 0;
   nonia = 0;

   vProb = -1.;
   dimuon_sv_x = 0.;
   dimuon_sv_y = 0.;
   dimuon_sv_z = 0.;

   dimuon_p4.SetPtEtaPhiM(0., 0., 0., 0.);
   muonP_p4.SetPtEtaPhiM(0., 0., 0., 0.);
   muonM_p4.SetPtEtaPhiM(0., 0., 0., 0.);
   gen_dimuon_p4.SetPtEtaPhiM(0., 0., 0., 0.);
   gen_muonP_p4.SetPtEtaPhiM(0., 0., 0., 0.);
   gen_muonM_p4.SetPtEtaPhiM(0., 0., 0., 0.);

   // Pruned particles are the one containing "important" stuff
   edm::Handle<reco::GenParticleCollection> pruned;
   iEvent.getByToken(genCands_, pruned);

   // Packed particles are all the status 1. The navigation to pruned is possible (the other direction should be made by hand)
   edm::Handle<pat::PackedGenParticleCollection> packed;
   iEvent.getByToken(packCands_, packed);

   bool gen_found = false;  //EDIT
   if ((isMC_ || only_gen_) && packed.isValid() && pruned.isValid())
   {
      for (size_t i = 0; i < pruned->size(); i++)
      {
         const reco::Candidate *aonia = &(*pruned)[i];
         if (((abs(aonia->pdgId()) == pdgid_) || (abs(aonia->pdgId()) == 100000 + pdgid_) || (abs(aonia->pdgId()) == 200000 + pdgid_)) && (aonia->status() == 2))
         {
            int foundit = 1;
            dimuon_pdgId = aonia->pdgId();
            for (size_t j = 0; j < packed->size(); j++)
            { // get the pointer to the first survied ancestor of a given packed GenParticle in the prunedCollection
               const reco::Candidate *motherInPrunedCollection = (*packed)[j].mother(0);
               const reco::Candidate *d = &(*packed)[j];
               if (motherInPrunedCollection != nullptr && (d->pdgId() == 13) && isAncestor(aonia, motherInPrunedCollection))
               {
                  gen_muonM_p4.SetPtEtaPhiM(d->pt(), d->eta(), d->phi(), d->mass());
                  foundit++;
               }
               if (motherInPrunedCollection != nullptr && (d->pdgId() == -13) && isAncestor(aonia, motherInPrunedCollection))
               {
                  gen_muonP_p4.SetPtEtaPhiM(d->pt(), d->eta(), d->phi(), d->mass());
                  foundit++;
               }
               if (foundit == 3)
                  break;
            }
            if (foundit == 3)
            {
               gen_dimuon_p4.SetPtEtaPhiM(aonia->pt(), aonia->eta(), aonia->phi(), aonia->mass());
               mother_pdgId = GetAncestor(aonia)->pdgId();
               gen_found = true;  //EDIT
               break;
            }
            else
               dimuon_pdgId = 0;
         } // if ( p_id
      } // for (size
      if (!dimuon_pdgId)
         std::cout << "MMrootupler: does not found the given decay " << run << "," << event << std::endl; // sanity check
   } // end if isMC

   float OniaMassMax_ = OniaMassCuts_[1];
   float OniaMassMin_ = OniaMassCuts_[0];

   bool already_stored = false;
   if (!only_gen_)
   { // we will look for dimuons
      if (dimuons.isValid() && !dimuons->empty())
      {
         for (pat::CompositeCandidateCollection::const_iterator TheDimuon = dimuons->begin(); TheDimuon != dimuons->end(); ++TheDimuon)
         {
            vProb = TheDimuon->userFloat("vProb");
            charge = TheDimuon->charge();
            if (vProb > -1. && TheDimuon->mass() > OniaMassMin_ && TheDimuon->mass() < OniaMassMax_ && charge == 0)
            {
               dimuon_p4.SetPtEtaPhiM(TheDimuon->pt(), TheDimuon->eta(), TheDimuon->phi(), TheDimuon->mass());
               reco::Candidate::LorentzVector vP = TheDimuon->daughter("muon1")->p4();
               reco::Candidate::LorentzVector vM = TheDimuon->daughter("muon2")->p4();
               if (TheDimuon->daughter("muon1")->charge() < 0)
               {
                  vP = TheDimuon->daughter("muon2")->p4();
                  vM = TheDimuon->daughter("muon1")->p4();
               }
               muonP_p4.SetPtEtaPhiM(vP.pt(), vP.eta(), vP.phi(), vP.mass());
               muonM_p4.SetPtEtaPhiM(vM.pt(), vM.eta(), vM.phi(), vM.mass());
               dimuon_sv_x = TheDimuon->vx();
               dimuon_sv_y = TheDimuon->vy();
               dimuon_sv_z = TheDimuon->vz();
               DCA = TheDimuon->userFloat("DCA");
               ppdlPV = TheDimuon->userFloat("ppdlPV");
               ppdlErrPV = TheDimuon->userFloat("ppdlErrPV");
               cosAlpha = TheDimuon->userFloat("cosAlpha");
               ppdlBS = TheDimuon->userFloat("ppdlBS");
               ppdlErrBS = TheDimuon->userFloat("ppdlErrBS");
               cosAlphaBS = TheDimuon->userFloat("cosAlphaBS");

               nonia++;
               if (only_best_)
                  break;
               else
               {
                  mm_tree->Fill(); // be aware, we are storing all combinations
                  already_stored = true;
               }
            }
            vProb = -1.;
         }
      } // if ( dimuons.isValid()

   } // !only_gen_

   if (!already_stored)
   { // we have to make sure, we are not double storing a combination
      if (!only_gen_)
      {
         if (nonia > 0) mm_tree->Fill(); // if not MC filter out
         else if (nonia == 0 && isMC_ && gen_found) mm_tree->Fill(); //EDIT
      }
      else
         mm_tree->Fill();
   }
}

void MMrootupler::fillDescriptions(edm::ConfigurationDescriptions &descriptions)
{
   edm::ParameterSetDescription desc;
   // desc.setUnknown();
   desc.add<edm::InputTag>("dimuons");
   desc.add<edm::InputTag>("primaryVertices");
   desc.add<edm::InputTag>("TriggerResults");
   desc.add<uint32_t>("onia_pdgid");
   desc.add<std::vector<double>>("onia_mass_cuts");
   desc.add<std::vector<std::string>>("FilterNames");
   desc.add<bool>("isMC");
   desc.add<bool>("only_best");
   desc.add<bool>("only_gen");

   descriptions.addDefault(desc);
}

DEFINE_FWK_MODULE(MMrootupler);
