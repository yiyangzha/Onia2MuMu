#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/PatCandidates/interface/CompositeCandidate.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "CommonTools/UtilAlgos/interface/StringCutObjectSelector.h"

class MMfilter : public edm::stream::EDProducer<>
{
public:
   explicit MMfilter(const edm::ParameterSet &);
   static void fillDescriptions(edm::ConfigurationDescriptions &);

private:
   void produce(edm::Event &, const edm::EventSetup &) override;
   UInt_t isTriggerMatched(const pat::CompositeCandidate);
   edm::EDGetTokenT<pat::CompositeCandidateCollection> dimuons_;
   StringCutObjectSelector<reco::Candidate, true> muonselection_;
   StringCutObjectSelector<reco::Candidate, true> mmselection_;
   bool do_trigger_match_;
   std::vector<std::string> HLT_filters_;
};

MMfilter::MMfilter(const edm::ParameterSet &iConfig) : dimuons_(consumes<pat::CompositeCandidateCollection>(iConfig.getParameter<edm::InputTag>("dimuons"))),
                                                       muonselection_(iConfig.existsAs<std::string>("muonSelection") ? iConfig.getParameter<std::string>("muonSelection") : ""),
                                                       mmselection_(iConfig.existsAs<std::string>("mmSelection") ? iConfig.getParameter<std::string>("mmSelection") : ""),
                                                       do_trigger_match_(iConfig.getParameter<bool>("do_trigger_match")),
                                                       HLT_filters_(iConfig.getParameter<std::vector<std::string>>("HLT_filters"))
{
   produces<pat::CompositeCandidateCollection>();
}

UInt_t MMfilter::isTriggerMatched(const pat::CompositeCandidate the_candidate)
{
   const pat::Muon *muon1 = dynamic_cast<const pat::Muon *>(the_candidate.daughter("muon1"));
   const pat::Muon *muon2 = dynamic_cast<const pat::Muon *>(the_candidate.daughter("muon2"));
   UInt_t matched = 0; // if no list is given, is not matched

   // if matched a given trigger, set the bit, in the same order as listed
   for (unsigned int iTr = 0; iTr < HLT_filters_.size(); iTr++)
   {
      const pat::TriggerObjectStandAlone *mu1obj = muon1->triggerObjectMatchByPath(HLT_filters_[iTr], true, true);
      const pat::TriggerObjectStandAlone *mu2obj = muon2->triggerObjectMatchByPath(HLT_filters_[iTr], true, true);
      if ((mu1obj != nullptr) && (mu2obj != nullptr))
         matched += (1 << iTr);
   }
   return matched;
}

// ------------ method called to produce the data  ------------
void MMfilter::produce(edm::Event &iEvent, const edm::EventSetup &iSetup)
{
   std::unique_ptr<pat::CompositeCandidateCollection> mumu_output(new pat::CompositeCandidateCollection);
   edm::Handle<pat::CompositeCandidateCollection> dimuons;
   iEvent.getByToken(dimuons_, dimuons);
   if (dimuons.isValid() && !dimuons->empty())
   {
      for (pat::CompositeCandidateCollection::const_iterator the_dimuon = dimuons->begin(); the_dimuon != dimuons->end(); ++the_dimuon)
      {
         if (mmselection_(*the_dimuon) &&
             muonselection_(*the_dimuon->daughter("muon1")) &&
             muonselection_(*the_dimuon->daughter("muon2")) &&
             (!do_trigger_match_ || isTriggerMatched(*the_dimuon)))
            mumu_output->push_back(*the_dimuon);
      }
   }
   iEvent.put(std::move(mumu_output));
}

void MMfilter::fillDescriptions(edm::ConfigurationDescriptions &iDescriptions)
{
   edm::ParameterSetDescription desc;

   desc.add<edm::InputTag>("dimuons");
   desc.add<std::string>("muonSelection");
   desc.add<std::string>("mmSelection");
   desc.add<bool>("do_trigger_match");
   desc.add<std::vector<std::string>>("HLT_filters");

   iDescriptions.addDefault(desc);
}

// define this as a plug-in
DEFINE_FWK_MODULE(MMfilter);
