import pymel.core as pm
import logging

log = logging.getLogger("ui")

class BaseTemplate(pm.ui.AETemplate):
    
    def addControl(self, control, label=None, **kwargs):
        pm.ui.AETemplate.addControl(self, control, label=label, **kwargs)
        
    def beginLayout(self, name, collapse=True):
        pm.ui.AETemplate.beginLayout(self, name, collapse=collapse)
        

class AEBasicBSDFTemplate(BaseTemplate):
    def __init__(self, nodeName):
        BaseTemplate.__init__(self,nodeName)
        log.debug("AEBasicBSDFTemplate")
        self.thisNode = None
        self.node = pm.PyNode(self.nodeName)
        pm.mel.AEswatchDisplay(nodeName)
        self.beginScrollLayout()
        self.buildBody(nodeName)
        self.addExtraControls("ExtraControls")
        self.endScrollLayout()
        
    def buildBody(self, nodeName):
        self.thisNode = pm.PyNode(nodeName)
        self.beginLayout("ShaderSettings" ,collapse=0)
        self.beginNoOptimize()
        #autoAddBegin
        self.addControl("normalMapping", label="NormalMapping")
        self.addControl("translucentColor", label="Translucent Color")
        self.addControl("reflectanceColor", label="Reflectance Color")
        self.addControl("kappa", label="Kappa")
        self.addControl("diffuseColor", label="Diffuse Color")
        self.addControl("bump", label="Bump")
        self.addControl("anisotropy", label="Anisotropy")
        self.addControl("absorptionColor", label="AbsorptionColor")
        self.addControl("absorptionDensity", label="AbsorptionDensity")
        self.addControl("ior", label="Ior")
        self.addControl("roughness", label="Roughness")
        self.addControl("microRoughnessHeight", label="MicroRoughnessHeight")
        self.addControl("traceReflections", label="Trace Reflections")
        self.addControl("microRoughness", label="MicroRoughness")
        self.addControl("rotation", label="Rotation")
        self.addControl("reflectance90Deg", label="Reflectance 90 Degree")
        self.addControl("sigma", label="Sigma")
        self.addControl("microRoughnessWidth", label="MicroRoughnessWidth")
        self.addSeparator()
        #autoAddEnd
        self.endNoOptimize()
        self.endLayout()
        