from CRABClient.UserUtilities import config
config = config()

myname='CMSDAS_Upsilon_2025E_ParkingDoubleMuonLowMass0_v1'
mydata='/ParkingDoubleMuonLowMass0/Run2025E-PromptReco-v1/MINIAOD'
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

config.Data.inputDataset = mydata
config.Data.inputDBS = 'global'
config.Data.splitting = 'FileBased'
config.Data.unitsPerJob = 1
config.Data.lumiMask = 'Cert_Collisions2025_391658_398903_Muon.json'

config.Data.outLFNDirBase = '/store/user/<user_name>/CMSDAS/'
config.Data.publication = False
config.Data.outputDatasetTag  = myname
config.Site.storageSite = 'T2_CN_Beijing'
