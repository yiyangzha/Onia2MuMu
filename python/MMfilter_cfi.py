import FWCore.ParameterSet.Config as cms

MMfiltered = cms.EDProducer('MMfilter',
      dimuons           = cms.InputTag("MMproducer"),
      muonSelection     = cms.string(""),
      mmSelection       = cms.string("pt > 0. && charge==0"),
      do_trigger_match  = cms.bool(True),
      HLT_filters       = cms.vstring(
        'HLT_Dimuon10_Upsilon_y1p4_v*',                         #  1=        1
      ),
)
