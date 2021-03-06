import pymel.core as pm
import logging

log = logging.getLogger("ui")

class BaseTemplate(pm.ui.AETemplate):
    
    def addControl(self, control, label=None, **kwargs):
        pm.ui.AETemplate.addControl(self, control, label=label, **kwargs)
        
    def beginLayout(self, name, collapse=True):
        pm.ui.AETemplate.beginLayout(self, name, collapse=collapse)
        

class AEasPhysical_surface_shaderTemplate(BaseTemplate):
    def __init__(self, nodeName):
        BaseTemplate.__init__(self,nodeName)
        log.debug("AEasPhysical_surface_shaderTemplate")
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
        self.addControl("back_lighting_samples", label="Back Lighting Samples")
        self.addControl("front_lighting_samples", label="Front Lighting Samples")
        self.addControl("aerial_persp_sky_color", label="Aerial Perspective Sky Color")
        self.addControl("aerial_persp_mode", label="Aerial Perspective Mode")
        self.addControl("aerial_persp_distance", label="Aerial Perspective Distance")
        self.addControl("aerial_persp_intensity", label="Aerial Perspective Intensity")
        self.addControl("translucency", label="Translucency")
        self.addControl("color_multiplier", label="Color Multiplier")
        self.addControl("alpha_multiplier", label="Alpha Multiplier")
        self.addSeparator()
        #autoAddEnd
        self.endNoOptimize()
        self.endLayout()
        