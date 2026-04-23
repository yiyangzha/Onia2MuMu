from CRABClient.UserUtilities import config
config = config()

isMC = False
myname='CMSDAS_Upsilon_2022F_ParkingDoubleMuonLowMass0_v1'
if isMC:
    mydata='/Upsilonto2Mu_UpsilonFilter_2MuFilter_TuneCP5_13p6TeV_pythia8/Run3Summer22EEMiniAODv3-124X_mcRun3_2022_realistic_postEE_v1-v2/MINIAODSIM'
else:
    mydata='/ParkingDoubleMuonLowMass0/Run2022F-PromptReco-v1/MINIAOD'
myrun ='run_upsilon.py'

import datetime, time
ts = time.time()
st = datetime.datetime.fromtimestamp(ts).strftime('-%y%m%d-%H%M')

config.General.requestName = myname
config.General.workArea = 'CernJobs'
config.General.transferOutputs = True
config.General.transferLogs = True

config.JobType.pluginName = 'Analysis'
config.JobType.psetName = myrun
config.JobType.outputFiles = ['rootuple.root']
config.JobType.pyCfgParams = ['isMC={0}'.format('True' if isMC else 'False')]

config.Data.inputDataset = mydata
config.Data.inputDBS = 'global'
config.Data.splitting = 'FileBased'
config.Data.unitsPerJob = 1
config.Data.totalUnits = config.Data.unitsPerJob * 100
if not isMC:
    config.Data.lumiMask = 'Cert_Collisions2022_355100_362760_Muon.json'

config.Data.outLFNDirBase = '/store/user/<user_name>/CMSDAS/'
config.Data.publication = True
config.Data.outputDatasetTag  = myname
config.Site.storageSite = 'T2_CN_Beijing'
