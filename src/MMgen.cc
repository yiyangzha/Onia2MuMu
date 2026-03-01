#include <memory>
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/HepMCCandidate/interface/GenParticleFwd.h"
#include "TLorentzVector.h"
#include "TTree.h"
#include <vector>

class MMgen : public edm::stream::EDAnalyzer<> {
   public:
      explicit MMgen(const edm::ParameterSet&);
      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

   private:
      void  analyze(const edm::Event&, const edm::EventSetup&) override;
      const reco::Candidate* GetStableParticle(const reco::Candidate*);
      const reco::Candidate* GetAncestor(const reco::Candidate*);
      edm::EDGetTokenT<reco::GenParticleCollection> genCands_;
      int   onia_pdgid_;


      UInt_t    run;
      ULong64_t event;
      UInt_t    lumiblock;
      UInt_t    ncombo;

      TLorentzVector gen_muonP_p4;
      TLorentzVector gen_muonM_p4;
      TLorentzVector gen_dimuon_p4;
      Int_t mother_pdgId,dimuon_pdgId;

      TTree* onia_tree;
};

MMgen::MMgen(const edm::ParameterSet& iConfig):
   genCands_(consumes<reco::GenParticleCollection>(iConfig.getParameter<edm::InputTag>("src"))),
   onia_pdgid_(iConfig.getParameter<uint32_t>("onia_pdgid"))
{
   edm::Service < TFileService > fs;
   onia_tree = fs->make < TTree > ("oniaTree", "Tree of Onia2MuMu");
   onia_tree->Branch("run",      &run,      "run/i");
   onia_tree->Branch("event",    &event,    "event/l");
   onia_tree->Branch("lumiblock",&lumiblock,"lumiblock/i");
   onia_tree->Branch("ncombo",   &ncombo,   "ncombo/i");

   std::cout << "Onia2MuMuRootupler::Onia2MuMuRootupler: Onia id " << onia_pdgid_ << std::endl;
   onia_tree->Branch("mother_pdgId",  &mother_pdgId,     "mother_pdgId/I");
   onia_tree->Branch("dimuon_pdgId",  &dimuon_pdgId,     "dimuon_pdgId/I");
   onia_tree->Branch("gen_dimuon_p4", "TLorentzVector",  &gen_dimuon_p4);
   onia_tree->Branch("gen_muonP_p4",  "TLorentzVector",  &gen_muonP_p4);
   onia_tree->Branch("gen_muonN_p4",  "TLorentzVector",  &gen_muonM_p4);

   //genCands_ = consumes<reco::GenParticleCollection>((edm::InputTag)"GenParticles");
   //genCands_ = consumes<reco::GenParticleCollection>((edm::InputTag)"prunedGenParticles");
}

const reco::Candidate* MMgen::GetAncestor(const reco::Candidate* p) {
   if (p->numberOfMothers()) {
      if  ((p->mother(0))->pdgId() == p->pdgId()) return GetAncestor(p->mother(0));
      else return p->mother(0);
   }
   std::cout << "GetAncestor: Inconsistet ancestor, particle does not have a mother " << std::endl;
   return p;
}

const reco::Candidate* MMgen::GetStableParticle(const reco::Candidate* p) {
   if (p->status() == 1) return p;
   int n = p->numberOfDaughters();
   if ( n > 0 ) {
      for (int j = 0; j < n; ++j) {
	 const  reco::Candidate* d = p->daughter(j);
	 if (d->pdgId() == p->pdgId()) return GetStableParticle(d);
      }
   } 
   std::cout << "GetStableParticle: Inconsistent state of particle, it has not daugthers, but is unstable " << std::endl;
   return p;  
}

void MMgen::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {

   edm::Handle<reco::GenParticleCollection> GenParticles;
   iEvent.getByToken(genCands_, GenParticles);

   run       = iEvent.id().run();
   event     = iEvent.id().event();
   lumiblock = iEvent.id().luminosityBlock();

   dimuon_pdgId = 0;
   mother_pdgId = 0;

   gen_dimuon_p4.SetPtEtaPhiM(0.,0.,0.,0.);
   gen_muonP_p4.SetPtEtaPhiM(0.,0.,0.,0.);
   gen_muonM_p4.SetPtEtaPhiM(0.,0.,0.,0.);

   ncombo = 0;

   //std::cout << "GenParticles.isValid() = " << GenParticles.isValid() << std::endl;

   if ( GenParticles.isValid() ) {

      for ( reco::GenParticleCollection::const_iterator itParticle = GenParticles->begin(); itParticle != GenParticles->end(); ++itParticle ) {
	 Int_t pdgId = itParticle->pdgId();
	 dimuon_pdgId = 0;
	 int foundit = 0;
//	 std::cout << "mother id: " << pdgId << std::endl;
//	 std::cout << "mother status: " << itParticle->status() << std::endl;
	 if ( (abs(pdgId) == onia_pdgid_) && (itParticle->status() == 2) ) {
	    int n = itParticle->numberOfDaughters();
	    //std::cout << "number of daughters: " << n << std::endl;
	    if (n < 2) continue;

	    const  reco::Candidate* onia = &(*itParticle);
	    foundit++;
	    dimuon_pdgId = pdgId;

	    bool yetM = false;
	    bool yetP = false;

	    for (int j = 0; j < n; ++ j) {
	       const  reco::Candidate* d = itParticle->daughter(j);
	       Int_t dauId = d->pdgId();
	       //std::cout << "daughter id: " << dauId << std::endl;
	       if ( dauId == 13 && !yetM) {
		  const  reco::Candidate* mM = GetStableParticle(d);
		  gen_muonM_p4.SetPtEtaPhiM(mM->pt(),mM->eta(),mM->phi(),mM->mass());
		  foundit++;
		  yetM = true;
	       } 
	       if ( dauId == -13 && !yetP) {
		  const  reco::Candidate* mP = GetStableParticle(d);
		  gen_muonP_p4.SetPtEtaPhiM(mP->pt(),mP->eta(),mP->phi(),mP->mass());
		  foundit++;
		  yetP = true;
	       } 
	    }
	    if ( foundit == 3 ) {
	       mother_pdgId = GetAncestor(onia)->pdgId();
               gen_dimuon_p4.SetPtEtaPhiM(onia->pt(),onia->eta(),onia->phi(),onia->mass());
	    } else {
	       foundit = 0;
	       dimuon_pdgId = 0;
	       mother_pdgId = 0;
	    }
	 }  // if ( pdg
	 if (foundit == 3) {
	    onia_tree->Fill();
	    ncombo++; 
	 }

      }   // for ( reco
   }     // if (GenPar
   // sanity check
   if ( !ncombo ) std::cout << "MMgen: Decay not found [" << iEvent.id().run() << "," << iEvent.id().event() << "]" << std::endl; 
   //else onia_tree ->Fill();
}

void MMgen::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
   edm::ParameterSetDescription desc;
   desc.add<edm::InputTag>("src");
   desc.add<uint32_t>("onia_pdgid");

//   desc.setUnknown();

   descriptions.addDefault(desc);
}

DEFINE_FWK_MODULE(MMgen);
