import FWCore.ParameterSet.Config as cms

rootuple = cms.EDAnalyzer('MMrootupler',
      dimuons         = cms.InputTag("MMproducer"),
      primaryVertices = cms.InputTag("offlineSlimmedPrimaryVertices"),
      TriggerResults  = cms.InputTag("TriggerResults", "", "HLT"),
      onia_pdgid      = cms.uint32(553),
      onia_mass_cuts  = cms.vdouble(0.,999.),
      FilterNames     = cms.vstring(
        'HLT_Dimuon10_Upsilon_y1p4',                         #  1=        1
      ),
      isMC            = cms.bool(False),
      only_best       = cms.bool(True),
      only_gen        = cms.bool(False)
)
