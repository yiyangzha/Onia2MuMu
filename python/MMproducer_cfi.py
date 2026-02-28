import FWCore.ParameterSet.Config as cms

MMproducer = cms.EDProducer('MMproducer',
      muons              = cms.InputTag("slimmedMuons"),
      beamspot           = cms.InputTag("offlineBeamSpot"),
      primaryvertices    = cms.InputTag("offlineSlimmedPrimaryVertices"),
      dimuonselection    = cms.string(""),                              ## The dimuon must pass this selection before vertexing
      resolvePUambiguity = cms.bool(True)                               ## Order PVs by their vicinity to the J/psi vertex, not by sumPt                            
)
