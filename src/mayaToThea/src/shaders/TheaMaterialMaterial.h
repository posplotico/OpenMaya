#include <maya/MPxNode.h>
#include <maya/MTypeId.h>

// Plugin TheaMaterial Shader Class //


// This class will create a new shader. Shaders are custom dependency
// graph objects so we will derive this class from the basic DG node
// type MPxNode
//

class TheaMaterial : public MPxNode
{
public:
                    TheaMaterial();
    virtual         ~TheaMaterial();

    static  void *  creator();
    virtual MStatus compute( const MPlug&, MDataBlock& );
    static  MStatus initialize();


    // postConstructor:
    // The postConstructor method allows us to call MPxNode member
    // functions during initialization. Internally maya creates two
    // objects when a user defined node is created, the internal MObject
    // and the user derived object. The association between the these
    // two objects is not made until after the MPxNode constructor is
    // called. This implies that no MPxNode member function can be called
    // from the MPxNode constructor. The postConstructor will get called
    // immediately after the constructor when it is safe to call any
    // MPxNode member function.
    //

    virtual void    postConstructor();

    static  MTypeId   id;  // The IFF type id

protected:

// layering
	static MObject  layerWeight;
	static MObject  layerTexture;
	static MObject  layerShader;
	static MObject  layerName;
	static MObject  layers;

//---------------------------- automatically created attributes start ------------------------------------
	static    MObject emitterAccuracy;
	static    MObject ambientEmitter;
	static    MObject twosided;
	static    MObject emitterMaxRays;
	static    MObject shadowCatcher;
	static    MObject tracingDepth;
	static    MObject passiveEmitter;
	static    MObject bsdf;
	static    MObject perceptualLevel;
	static    MObject emitterMinRays;
	static    MObject emitter;
	static    MObject ambientLevel;
	static    MObject activeDirt;
//---------------------------- automatically created attributes end ------------------------------------

   // Translucence coefficient
   static MObject  aTranslucenceCoeff;

   // Diffuse Reflectivity
   static MObject  aDiffuseReflectivity;

   // Surface color
   static MObject  aColor;

   // Incandescence
   static MObject  aIncandescence;

   // Surface transparency
   static MObject  aInTransparency;

   // Output color
   static MObject  aOutColor;

   // Output transparency
   static MObject  aOutTransparency;

   // X component of surface normal
   static MObject  aNormalCameraX;

   // Y component of surface normal
   static MObject  aNormalCameraY;

   // Z component of surface normal
   static MObject  aNormalCameraZ;

   // Surface normal
   static MObject  aNormalCamera;

   // X component of light direction vector
   static MObject  aLightDirectionX;

   // Y component of light direction vector
   static MObject  aLightDirectionY;

   // Z component of light direction vector
   static MObject  aLightDirectionZ;

   // Light direction vector
   static MObject  aLightDirection;

   // Red component of light intensity
   static MObject  aLightIntensityR;

   // Green component of light intensity
   static MObject  aLightIntensityG;

   // Blue component of light intensity
   static MObject  aLightIntensityB;

   // Light Intensity vector
   static MObject  aLightIntensity;

   // Ambient flag
   static MObject  aLightAmbient;

   // Diffuse flag
   static MObject  aLightDiffuse;

   // Specular flag
   static MObject  aLightSpecular;

   // Shadow Fraction flag
   static MObject   aLightShadowFraction;

   // Pre Shadow Intensity
   static MObject   aPreShadowIntensity;

   // Light blind data
   static MObject   aLightBlindData;

   // Light data array
   static MObject   aLightData;

};
