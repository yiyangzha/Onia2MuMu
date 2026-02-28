import FWCore.ParameterSet.Config as cms

MMgen = cms.EDAnalyzer('MMgen',
    src = cms.InputTag('GenParticles'),
    onia_pdgid = cms.uint32(553)
)
