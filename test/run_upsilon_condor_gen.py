from FWCore.ParameterSet.VarParsing import VarParsing
options = VarParsing ('analysis')
options.parseArguments()

ouput_filename = 'rootuple.root'

import FWCore.ParameterSet.Config as cms
process = cms.Process("Rootuple")

process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger = cms.Service("MessageLogger",
    cerr = cms.untracked.PSet(
        threshold = cms.untracked.string('ERROR')
    )
)

process.maxEvents    = cms.untracked.PSet(input = cms.untracked.int32(-1))
process.source       = cms.Source("PoolSource", fileNames = cms.untracked.vstring(options.inputFiles))

process.TFileService = cms.Service("TFileService", fileName = cms.string(ouput_filename))
process.options      = cms.untracked.PSet( wantSummary = cms.untracked.bool(False) )

process.rootuple = cms.EDAnalyzer('MMgen',
    src = cms.InputTag('genParticles'),
    onia_pdgid = cms.uint32(553)
)

process.p = cms.Path(process.rootuple)
