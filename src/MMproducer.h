#ifndef _MMproducer_h
#define _MMproducer_h

// system include files
#include <memory>

// FW include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

// DataFormat includes
#include <DataFormats/PatCandidates/interface/CompositeCandidate.h>
#include <DataFormats/PatCandidates/interface/Muon.h>

#include <CommonTools/UtilAlgos/interface/StringCutObjectSelector.h>

#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "MagneticField/Engine/interface/MagneticField.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"

template <typename T>
struct GreaterByVProb {
   bool operator()(const T& t1, const T& t2) const { return t1.userFloat("vProb") > t2.userFloat("vProb"); }
};

// class decleration

class MMproducer : public edm::stream::EDProducer<> {
   public:
      explicit MMproducer(const edm::ParameterSet&);
      static void fillDescriptions(edm::ConfigurationDescriptions&);

   private:
      void produce(edm::Event&, const edm::EventSetup&) override;

   private:
      edm::EDGetTokenT<pat::MuonCollection>          muons_;
      edm::EDGetTokenT<reco::BeamSpot>               thebeamspot_;
      edm::EDGetTokenT<reco::VertexCollection>       thePVs_;
      edm::ESGetToken<MagneticField, IdealMagneticFieldRecord>     magneticField_;
      edm::ESGetToken<TransientTrackBuilder, TransientTrackRecord> theTTBuilder_;
      StringCutObjectSelector<reco::Candidate, true> dimuonSelection_;
      bool                                           resolveAmbiguity_;
      GreaterByVProb<pat::CompositeCandidate>        vPComparator_;
};

#endif
