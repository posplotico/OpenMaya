#include <fstream>
#include "CoronaShaders.h"
#include "../mtco_common/mtco_mayaObject.h"
#include "shadingtools/material.h"
#include "shadingtools/shaderDefs.h"
#include "shadingtools/shadingNode.h"
#include "utilities/tools.h"
#include "utilities/attrTools.h"
#include "utilities/logging.h"
#include "osl/oslUtils.h"
#include "../coronaOSL/coronaOSLMapUtil.h"
#include "CoronaUtils.h"
#include "world.h"
#include "renderGlobals.h"

static Logging logger;

void clearDataList()
{
	dataList.clear();
}


Corona::Rgb defineColor(MString& attributeName, MFnDependencyNode& depFn)
{
	MColor col = getColorAttr(attributeName.asChar(), depFn);
	return Corona::Rgb(col.r, col.g, col.b);
}

float defineFloat(MString& attributeName, MFnDependencyNode& depFn)
{
	return getFloatAttr(attributeName.asChar(), depFn, 0.0f);
}

// if we have something like a color attribute which we want to parse, we derive the 
// shading network from the attribute name and depFn
Corona::ColorOrMap defineAttribute(MString& attributeName, MObject& node)
{
	ShadingNetwork network(node);
	MFnDependencyNode depFn(node);

	return defineAttribute(attributeName, depFn, network);
}

Corona::ColorOrMap defineAttribute(MString& attributeName, MFnDependencyNode& depFn, ShadingNetwork& sn)
{
	MStatus stat;

	Corona::SharedPtr<Corona::Abstract::Map> texmap = nullptr;

	Corona::Rgb rgbColor(0.0);
	//Logging::debug(MString("check if : ") + depFn.name() + "." + attributeName + " is connected");

	if (isConnected(attributeName.asChar(), depFn, true, true))
	{
		//Logging::debug(MString("it is connected"));
 		rgbColor = Corona::Rgb(0.0, 0.0, 1.0);
		texmap = getOslTexMap(attributeName, depFn, sn);
	}
	else{
		if (getPlugAttrType(attributeName.asChar(), depFn) == ATTR_TYPE::ATTR_TYPE_COLOR)
		{
			MColor col = getColorAttr(attributeName.asChar(), depFn);
			MString multiplierName = attributeName + "Multiplier";
			MPlug multiplierPlug = depFn.findPlug(multiplierName, &stat);
			float multiplier = 1.0f;
			if (stat)
				multiplier = multiplierPlug.asFloat();
			Corona::Rgb offsetColor(0,0,0);
			rgbColor = Corona::Rgb(col.r * multiplier, col.g * multiplier, col.b * multiplier);
			rgbColor += offsetColor;
			
		}
		if (getPlugAttrType(attributeName.asChar(), depFn) == ATTR_TYPE::ATTR_TYPE_FLOAT)
		{
			float f = getFloatAttr(attributeName.asChar(), depFn, 0.0f);
			MString multiplierName = attributeName + "Multiplier";
			MPlug multiplierPlug = depFn.findPlug(multiplierName);
			if (!multiplierPlug.isNull())
				f *= multiplierPlug.asFloat();
			rgbColor = Corona::Rgb(f, f, f);
		}
	}
	return Corona::ColorOrMap(rgbColor, texmap);
}

Corona::SharedPtr<Corona::Abstract::Map> defineBump(MString& attributeName, MFnDependencyNode& depFn, ShadingNetwork& sn)
{
	Corona::SharedPtr<Corona::Abstract::Map> texmap = nullptr;
	if (isConnected("normalCamera", depFn, true, false))
	{
		MString normalCamName = "normalCamera";
		Logging::debug(MString("normal camera is connected"));
		texmap = getOslTexMap(normalCamName, depFn, sn);
		Logging::debug("Bump connected");
		return texmap;
	}
	return nullptr;
}


Corona::SharedPtr<Corona::IMaterial> defineDefaultMaterial()
{
	Corona::NativeMtlData data;
	data.components.diffuse.setColor(Corona::Rgb(.7, .7, .7));
	return data.createMaterial();
}

Corona::SharedPtr<Corona::IMaterial> defineCoronaMaterial(MObject& materialNode, std::shared_ptr<MayaObject> obj)
{
	float globalScaleFactor = 1.0f;
	if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr != nullptr)
		globalScaleFactor = MayaTo::getWorldPtr()->worldRenderGlobalsPtr->scaleFactor;

	MAYATO_OSL::initOSLUtil();

	if (materialNode == MObject::kNullObj)
		return defineDefaultMaterial();

	ShadingNetwork network(materialNode);

	if (network.shaderList.size() == 0)
		return defineDefaultMaterial();

	Logging::debug(MString("Defining corona material from node: ") + network.rootNodeName);

	MFnDependencyNode depFn(materialNode);

	// if we do a swatch render, we always want to create a new data object
	if (MayaTo::getWorldPtr()->getRenderState() != MayaTo::MayaToWorld::WorldRenderState::RSTATESWATCHRENDERING)
	{
		for (size_t i = 0; i < dataList.size(); i++)
		{
			if (dataList[i].mtlName == depFn.name())
				return dataList[i].data.createMaterial();
		}
	}

	if (depFn.typeName() == "CoronaLayered")
	{
		Logging::debug(MString("Defining layered material: ") + depFn.name());
		MPlug baseMatPlug = depFn.findPlug("baseMaterial");
		if (baseMatPlug.isNull() || !baseMatPlug.isConnected())
			return defineDefaultMaterial();
		MPlugArray connected;
		baseMatPlug.connectedTo(connected, true, false);
		if (connected.length() == 0)
			return defineDefaultMaterial();
		MObject connectedMat = connected[0].node();
		MString connectedMatName = MFnDependencyNode(connectedMat).typeName();
		std::vector<std::string> validNames = {"CoronaSurface", "CoronaLight", "CoronaVolume"};
		bool found = false;
		for (auto n : validNames)
			if (connectedMatName == n.c_str())
				found = true;
		if (!found )
			return defineDefaultMaterial();

		Corona::NativeMtlData data;
		data.components.diffuse.setColor(Corona::Rgb(.7, .7, .7));

		Logging::debug(MString("Defining layered base Material: ") + connectedMatName);
		Corona::SharedPtr<Corona::IMaterial> connectedBaseMat = defineCoronaMaterial(connectedMat, obj);
		Corona::LayeredMtlData layerData;
		layerData.baseMtl = connectedBaseMat;
		Corona::LayeredMtlData::MtlEntry baseEntry;
		baseEntry.material = connectedBaseMat;
		baseEntry.amount = Corona::ColorOrMap(Corona::Rgb(1, 1, 1), nullptr);
		layerData.layers.push(baseEntry);

		MPlug entryPlug = depFn.findPlug("materialEntry");
		MPlug mtlEntryPlug = depFn.findPlug("materialEntryMtl");
		MPlug amountEntryPlug = depFn.findPlug("materialEntryAmount");
		
		Logging::debug(MString("Defining layers: "));
		if (mtlEntryPlug.numElements() == amountEntryPlug.numElements())
		{
			Logging::debug(MString("Found ") + mtlEntryPlug.numElements() + " material entries.");
			for (uint pId = 0; pId < mtlEntryPlug.numElements(); pId++)
			{
				MPlug mPlug = mtlEntryPlug[pId];
				MPlug aPlug = amountEntryPlug[pId];
				if (!mPlug.isConnected())
					continue;

				MPlugArray inputs;
				mPlug.connectedTo(inputs, true, false);
				if (inputs.length() == 0)
					continue;

				// check for valid connection
				MString matType = MFnDependencyNode(inputs[0].node()).typeName();
				MString nodeName = MFnDependencyNode(inputs[0].node()).name();
				bool found = false;
				for (auto n : validNames)
					if (matType == n.c_str())
						found = true;
				if (!found)
					continue;

				Logging::debug(MString("Creating entry ") + pId + " Shader: " + nodeName);
				Corona::LayeredMtlData::MtlEntry entry;
				entry.material = defineCoronaMaterial(inputs[0].node(), obj);
				MString attName = aPlug.name();
				MStringArray strArray;
				attName.split('.', strArray);
				if (strArray.length() > 1)
					attName = strArray[strArray.length()-1];
				entry.amount = defineAttribute(attName, materialNode);
				layerData.layers.push(entry);
			}
		}

		//mtlData md;
		//md.data = layerData;
		//md.mtlName = depFn.name();
		//dataList.push_back(md);

		return layerData.createMaterial();

		//return defineDefaultMaterial();
	}

	if (depFn.typeName() == "CoronaSurface")
	{
		Corona::NativeMtlData data;
		data.components.diffuse = defineAttribute(MString("diffuse"), depFn, network);
		data.components.translucency = defineAttribute(MString("translucency"), depFn, network);
		data.translucencyLevel = defineAttribute(MString("translucencyFraction"), depFn, network);
		//data.translucencyLevel = defineAttribute(MString("translucencyLevel"), depFn, network);
		data.components.reflect = defineAttribute(MString("reflectivity"), depFn, network);
		const Corona::BsdfLobeType bsdfType[] = { Corona::BSDF_ASHIKHMIN, Corona::BSDF_PHONG, Corona::BSDF_WARD };
		data.reflect.glossiness = defineAttribute(MString("reflectionGlossiness"), depFn, network);
		data.reflect.glossiness = defineAttribute(MString("reflectionGlossiness"), depFn, network);
		data.reflect.fresnelIor = defineAttribute(MString("fresnelIor"), depFn, network);
		data.reflect.anisotropy = defineAttribute(MString("anisotropy"), depFn, network);
		data.reflect.anisoRotation = defineAttribute(MString("anisotropicRotation"), depFn, network);
		data.components.refract = defineAttribute(MString("refractivity"), depFn, network);
		data.refract.ior = defineAttribute(MString("refractionIndex"), depFn, network);
		data.refract.glossiness = defineAttribute(MString("refractionGlossiness"), depFn, network);

		int glassType = getEnumInt("glassType", depFn);
		Corona::GlassMode glassModes[] = { Corona::GLASS_HYBRID, Corona::GLASS_ONESIDED, Corona::GLASS_TWOSIDED };
		data.refract.glassMode = glassModes[glassType];

		// round corners - without obj we are doing a swatch rendering. Here round corners does not make sense.
		if (obj != nullptr)
		{
			MPlug rcMultiPlug = depFn.findPlug("roundCornersRadiusMultiplier");
			if (!rcMultiPlug.isNull())
				rcMultiPlug.setFloat(globalScaleFactor);
			data.roundedCorners.radius = defineAttribute(MString("roundCornersRadius"), depFn, network);
			//data.roundedCorners.radius = rcRadius * globalScaleFactor;
			getInt(MString("roundCornersSamples"), depFn, data.roundedCorners.samples);
		}

		// --- volume ----
		data.volume.attenuationColor = defineAttribute(MString("attenuationColor"), depFn, network);
		data.volume.attenuationDist = defineFloat(MString("attenuationDist"), depFn);
		data.volume.attenuationDist *= globalScaleFactor;
		data.volume.emissionColor = defineColor(MString("volumeEmissionColor"), depFn);
		data.volume.emissionDist = defineFloat(MString("volumeEmissionDist"), depFn);

		//// -- volume sss --
		data.volume.meanCosine = getFloatAttr("volumeMeanCosine", depFn, 0.0f);
		data.volume.scatteringAlbedo = defineAttribute(MString("volumeScatteringAlbedo"), depFn, network);
		data.volume.sssMode = getBoolAttr("volumeSSSMode", depFn, false);

		data.opacity = defineAttribute(MString("opacity"), depFn, network);

		// ---- emission ----
		data.emission.color = defineAttribute(MString("emissionColor"), depFn, network);
		//bool disableSampling = false;
		//getBool("emissionDisableSampling", depFn, disableSampling);
		data.emission.disableSampling = true; // always disable sampling because we use it only in light shaders
		//bool sharpnessFake = false;
		//getBool("emissionSharpnessFake", depFn, sharpnessFake);
		//data.emission.sharpnessFake = sharpnessFake;
		//MVector point(0, 0, 0);
		//getPoint(MString("emissionSharpnessFakePoint"), depFn, point);
		//data.emission.sharpnessFakePoint = Corona::AnimatedPos(Corona::Pos(point.x, point.y, point.z));


		// ---- alpha mode ----
		int alphaMode = 0;
		getEnum(MString("alphaMode"), depFn, alphaMode);
		Corona::AlphaInteraction alphaInteraction[] = { Corona::ALPHA_DEFAULT, Corona::ALPHA_SOLID, Corona::ALPHA_TRANSPARENT };
		data.alpha = alphaInteraction[alphaMode];

		data.bump = defineBump(MString("bump"), depFn, network);

		// setup all object specific data which needs a mayaobject
		if (obj != nullptr)
		{
			// overrideColor is not yet reimplemented. It should be used by particle instancer
			MColor overrideColor(1, 1, 1);
			if (obj->attributes != nullptr)
				if (obj->attributes->hasColorOverride)
					overrideColor = obj->attributes->colorOverride;

			MFnDependencyNode geoFn(obj->mobject);

			// cast shadows is derived from geometry
			// this attribute is not exposed in the UI by default but a advanced user can use it if he wants to
			data.castsShadows = getBoolAttr("castsShadows", depFn, true);
			if (data.castsShadows)
			{
				if (!getBoolAttr("castsShadows", geoFn, true))
					data.castsShadows = false;
			}

			int shadowCatcherMode = getIntAttr("mtco_shadowCatcherMode", geoFn, 0);
			if (shadowCatcherMode > 0)
			{
				if (shadowCatcherMode == 1)
					data.shadowCatcherMode = Corona::SC_ENABLED_FINAL;
				if (shadowCatcherMode == 2)
					data.shadowCatcherMode = Corona::SC_ENABLED_COMPOSITE;
			}


			// ---- ies profiles -----
			MStatus stat;
			MPlug iesPlug = depFn.findPlug("iesProfile", &stat);
			if (stat)
			{
				//data.emission.gonioDiagram
				MString iesFile = iesPlug.asString();
				if (iesFile.length() > 4)
				{
					Corona::IesParser iesParser;
					Corona::FileReader input;
					Corona::String fn(iesFile.asChar());
					input.open(fn);
					if (!input.failed())
					{

						try {

							const double rm[4][4] = {
								{ 1.0, 0.0, 0.0, 0.0 },
								{ 0.0, 0.0, 1.0, 0.0 },
								{ 0.0, -1.0, 0.0, 0.0 },
								{ 0.0, 0.0, 0.0, 1.0 }
							};
							MMatrix zup(rm);

							MMatrix tm = zup * obj->transformMatrices[0];
							Corona::AnimatedAffineTm atm;
							setAnimatedTransformationMatrix(atm, tm);

							const Corona::IesLight light = iesParser.parseIesLight(input);
							data.emission.gonioDiagram = light.distribution;
							Corona::Matrix33 m(atm[0].extractRotation());

							data.emission.emissionFrame = Corona::AnimatedMatrix33(m);

						}
						catch (Corona::Exception& ex) {
							Logging::error(MString(ex.getMessage().cStr()));
						}
					}
					else{
						Logging::error(MString("Unable to read ies file .") + iesFile);
					}
				}
			}


		}
		mtlData md;
		md.data = data;
		md.mtlName = depFn.name();
		dataList.push_back(md);

		return data.createMaterial();
	}

	if (depFn.typeName() == "CoronaLight")
	{
		Corona::NativeMtlData data;
		// ---- emission ----
		data.emission.color = defineAttribute(MString("emissionColor"), depFn, network);
		//bool disableSampling = false;
		//getBool("emissionDisableSampling", depFn, disableSampling);
		data.opacity = defineAttribute(MString("opacity"), depFn, network);
		// if the CoronaLight shader is not supposed to emit light, it should be used as something like a self illuminated BG
		// so we turn off visible in GI and disableSampling. 
		if (getBoolAttr("emitLight", depFn, true))
		{
			data.emission.disableSampling = false;
			if (obj != nullptr)
			{
				if (!getBoolAttr("mtco_visibleInGI", depFn, true))
				{
					MFnDependencyNode geoDN(obj->mobject);
					MPlug giPlug = geoDN.findPlug("mtco_visibleInGI");
					giPlug.setBool(true);
				}
			}
		}
		else
		{
			data.emission.disableSampling = true;
			if (obj != nullptr)
			{
				//getBoolAttr("mtco_visibleInGI", depFn, true);
				MFnDependencyNode geoDN(obj->mobject);
				MPlug giPlug = geoDN.findPlug("mtco_visibleInGI");
				giPlug.setBool(false);
			}
		}
		// setup all object specific data which needs a mayaobject
		if (obj != nullptr)
		{
			bool sharpnessFake = false;
			getBool("emissionSharpnessFake", depFn, sharpnessFake);
			data.emission.sharpnessFake = sharpnessFake;
			//MVector point(0, 0, 0);
			MMatrix centerM = obj->transformMatrices[0];
			MPoint p(centerM[3][0], centerM[3][1], centerM[3][2]);
			//getPoint(MString("emissionSharpnessFakePoint"), depFn, point);
			data.emission.sharpnessFakePoint = Corona::AnimatedPos(Corona::Pos(p.x, p.y, p.z));

			// ---- ies profiles -----
			MStatus stat;
			MPlug iesPlug = depFn.findPlug("iesProfile", &stat);
			if (stat)
			{
				//data.emission.gonioDiagram
				MString iesFile = iesPlug.asString();
				if (iesFile.length() > 4)
				{
					std::ifstream iesFileObject;
					iesFileObject.open(iesFile.asChar());
					if (!(iesFileObject.is_open() && iesFileObject.good()))
					{
						iesFileObject.close();
					}
					else{
						iesFileObject.close();

						Corona::IesParser iesParser;
						Corona::FileReader input;
						Corona::String fn(iesFile.asChar());
						input.open(fn);
						if (!input.failed())
						{

							try {

								const double rm[4][4] = {
									{ 1.0, 0.0, 0.0, 0.0 },
									{ 0.0, 0.0, 1.0, 0.0 },
									{ 0.0, -1.0, 0.0, 0.0 },
									{ 0.0, 0.0, 0.0, 1.0 }
								};
								MMatrix zup(rm);

								MMatrix tm = zup * obj->transformMatrices[0];
								Corona::AnimatedAffineTm atm;
								setAnimatedTransformationMatrix(atm, tm);

								const Corona::IesLight light = iesParser.parseIesLight(input);
								data.emission.gonioDiagram = light.distribution;
								Corona::Matrix33 m(atm[0].extractRotation());

								data.emission.emissionFrame = Corona::AnimatedMatrix33(m);

							}
							catch (Corona::Exception& ex) {
								Logging::error(MString(ex.getMessage().cStr()));
							}
						}
						else{
							Logging::error(MString("Unable to read ies file .") + iesFile);
						}
					}
				}
			}
		}

		mtlData md;
		md.data = data;
		md.mtlName = depFn.name();
		dataList.push_back(md);
		return data.createMaterial();
	}

	if (depFn.typeName() == "CoronaVolume")
	{
		Corona::NativeMtlData data;
		// --- volume ----
		data.opacity = defineAttribute(MString("opacity"), depFn, network);

		data.volume.attenuationColor = defineAttribute(MString("attenuationColor"), depFn, network);
		data.volume.attenuationDist = defineFloat(MString("attenuationDist"), depFn);
		data.volume.attenuationDist *= globalScaleFactor;
		data.volume.emissionColor = defineColor(MString("volumeEmissionColor"), depFn);
		data.volume.emissionDist = defineFloat(MString("volumeEmissionDist"), depFn);

		//// -- volume sss --
		data.volume.meanCosine = getFloatAttr("volumeMeanCosine", depFn, 0.0f);
		data.volume.scatteringAlbedo = defineAttribute(MString("volumeScatteringAlbedo"), depFn, network);
		data.volume.sssMode = getBoolAttr("volumeSSSMode", depFn, false);

		mtlData md;
		md.data = data;
		md.mtlName = depFn.name();
		dataList.push_back(md);
		return data.createMaterial();
	}

	if (depFn.typeName() == "lambert")
	{
		Corona::NativeMtlData data;
		MColor col = getColorAttr("color", depFn);
		data.components.diffuse.setColor(Corona::Rgb(col.r, col.g, col.b));
		data.components.reflect.setColor(0.0f);
		mtlData md;
		md.data = data;
		md.mtlName = depFn.name();
		dataList.push_back(md);
		return data.createMaterial();
	}

	return defineDefaultMaterial();
}