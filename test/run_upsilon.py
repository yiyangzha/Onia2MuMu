from FWCore.ParameterSet.VarParsing import VarParsing
options = VarParsing ('analysis')
options.parseArguments()

ouput_filename = options.outputFile
input_filename = options.inputFiles

ouput_filename = 'rootuple.root'

import FWCore.ParameterSet.Config as cms
process = cms.Process("Rootuple")

process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.Reconstruction_cff')
process.load('Configuration.StandardSequences.Services_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '124X_dataRun3_v15', '')

process.MessageLogger = cms.Service("MessageLogger",
    cerr = cms.untracked.PSet(
        threshold = cms.untracked.string('ERROR')
    )
)

process.maxEvents    = cms.untracked.PSet(input = cms.untracked.int32(-1))
process.source       = cms.Source("PoolSource", fileNames = cms.untracked.vstring(input_filename))

process.TFileService = cms.Service("TFileService", fileName = cms.string(ouput_filename))
process.options      = cms.untracked.PSet( wantSummary = cms.untracked.bool(False) )

process.selectedMuons = cms.EDFilter('PATMuonSelector',
   src = cms.InputTag('slimmedMuons'),
   cut = cms.string(
    'muonID(\"TMOneStationTight\")'
	    ' && abs(innerTrack.dxy) < 0.3'
	    ' && abs(innerTrack.dz)  < 20.'
	    ' && innerTrack.hitPattern.trackerLayersWithMeasurement > 5'
	    ' && innerTrack.hitPattern.pixelLayersWithMeasurement > 0'
	    ' && innerTrack.quality(\"highPurity\")'
	    ' && (abs(eta) <= 2.4 && pt > 3.)'
   ),
   filter = cms.bool(True)
)

process.load("Analyzers.MuMu.MMproducer_cfi")
process.MMproducer.muons=cms.InputTag('selectedMuons')

process.load("Analyzers.MuMu.MMfilter_cfi")
process.MMfiltered.do_trigger_match = cms.bool(True)

process.MMcounter = cms.EDFilter('CandViewCountFilter',
    src       = cms.InputTag("MMfiltered"),
    minNumber = cms.uint32(1)
)

process.onia2MMSequence = cms.Sequence(
   process.selectedMuons *
   process.MMproducer *
   process.MMfiltered *
   process.MMcounter
)

process.load("Analyzers.MuMu.MMrootupler_cfi")
process.rootuple.dimuons = cms.InputTag("MMfiltered")
process.rootuple.onia_pdgid = cms.uint32(553)

process.p = cms.Path(process.onia2MMSequence * process.rootuple)
