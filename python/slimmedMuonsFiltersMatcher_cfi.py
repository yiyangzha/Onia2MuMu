import FWCore.ParameterSet.Config as cms

### ==== This is our version of the patMuonsWithTrigger using MINIAOD

unpackedPatTrigger = cms.EDProducer("PATTriggerObjectStandAloneUnpacker",
      patTriggerObjectsStandAlone = cms.InputTag( 'slimmedPatTrigger' ),
      triggerResults              = cms.InputTag( 'TriggerResults::HLT' ),
      unpackFilterLabels          = cms.bool( True )
      )

PATmuonTriggerMatchHLT = cms.EDProducer( "PATTriggerMatcherDRDPtLessByR",
      src     = cms.InputTag( "slimmedMuons" ),
      matched = cms.InputTag( "unpackedPatTrigger" ),
      matchedCuts = cms.string('coll("hltL3MuonCandidates")'),
      maxDPtRel = cms.double( 10. ),
      maxDeltaR = cms.double( 0.1 ),
      resolveAmbiguities    = cms.bool( True ),
      resolveByMatchQuality = cms.bool( True )
      )

PATmuonMatchIterL3 = PATmuonTriggerMatchHLT.clone(matchedCuts = cms.string('coll("hltIterL3MuonCandidates")')) 

slimmedMuonsTriggerMatchers1Mu = cms.Sequence(
      PATmuonTriggerMatchHLT +
      PATmuonMatchIterL3
      )

slimmedMuonsTriggerMatchers1MuInputTags = [
   cms.InputTag('PATmuonTriggerMatchHLT'),
   cms.InputTag('PATmuonMatchIterL3')
]

slimmedMuonsWithTrigger = cms.EDProducer( "PATTriggerMatchMuonEmbedder",
      src     = cms.InputTag(  "slimmedMuons" ),
      matches = cms.VInputTag()
      )

slimmedMuonsWithTrigger.matches += slimmedMuonsTriggerMatchers1MuInputTags

slimmedMuonsWithTriggerSequence = cms.Sequence(
      unpackedPatTrigger *
      slimmedMuonsTriggerMatchers1Mu *
      slimmedMuonsWithTrigger
      )
