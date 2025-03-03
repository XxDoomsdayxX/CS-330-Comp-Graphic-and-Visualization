///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	// free the allocated OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL &material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
	// Load texture images for the scene
	bool textureLoaded = false;

	// Example: Load up to 16 textures
	textureLoaded = CreateGLTexture("../../Utilities/textures/abstract.jpg", "abstractTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load abstract texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/backdrop.jpg", "backdropTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load backdrop texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/cotton.jpg", "cottonTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load breadcrust texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/parquet.jpg", "parquetTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load cheddar texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/cotton2.jpg", "cotton2Texture");
	if (!textureLoaded) {
		std::cout << "Failed to load cheese top texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/leaves.jpg", "leavesTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load cheese wheel texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/wood.jpg", "woodTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load circular brushed gold texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/drywall.jpg", "drywallTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load drywall texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/wicker-basket.jpg", "wickerBasketTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load gold seamless texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/knife_handle.jpg", "knifeHandleTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load knife handle texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/pavers.jpg", "paversTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load pavers texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "rusticWoodTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load rustic wood texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/stainedglass.jpg", "stainedGlassTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load stained glass texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/stainless.jpg", "stainlessTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load stainless texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/grey_fabric.jpg", "fabricTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load stainless end texture!" << std::endl;
	}
	textureLoaded = CreateGLTexture("../../Utilities/textures/tilesf2.jpg", "tilesTexture");
	if (!textureLoaded) {
		std::cout << "Failed to load tiles texture!" << std::endl;
	}

	// After the texture image data is loaded into memory, bind the textures
	BindGLTextures();
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "gold";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.ambientStrength = 0.3f;
	metalMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.shininess = 22.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL cheeseMaterial;
	cheeseMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cheeseMaterial.ambientStrength = 0.2f;
	cheeseMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cheeseMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cheeseMaterial.shininess = 0.3;
	cheeseMaterial.tag = "cheese";

	m_objectMaterials.push_back(cheeseMaterial);

	OBJECT_MATERIAL breadMaterial;
	breadMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	breadMaterial.ambientStrength = 0.3f;
	breadMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	breadMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	breadMaterial.shininess = 0.5;
	breadMaterial.tag = "bread";

	m_objectMaterials.push_back(breadMaterial);

	OBJECT_MATERIAL darkBreadMaterial;
	darkBreadMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	darkBreadMaterial.ambientStrength = 0.2f;
	darkBreadMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	darkBreadMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	darkBreadMaterial.shininess = 0.0;
	darkBreadMaterial.tag = "darkbread";

	m_objectMaterials.push_back(darkBreadMaterial);

	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	backdropMaterial.ambientStrength = 0.6f;
	backdropMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.1f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 0.0;
	backdropMaterial.tag = "backdrop";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL grapeMaterial;
	grapeMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	grapeMaterial.ambientStrength = 0.1f;
	grapeMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.3f);
	grapeMaterial.specularColor = glm::vec3(0.4f, 0.2f, 0.2f);
	grapeMaterial.shininess = 0.5;
	grapeMaterial.tag = "grape";

	m_objectMaterials.push_back(grapeMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";

	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";

	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";

	m_objectMaterials.push_back(clayMaterial);

	OBJECT_MATERIAL fabricCushionMaterial;
	fabricCushionMaterial.ambientColor = glm::vec3(0.3f, 0.25f, 0.2f);
	fabricCushionMaterial.ambientStrength = 0.4f;
	fabricCushionMaterial.diffuseColor = glm::vec3(0.6f, 0.55f, 0.5f);
	fabricCushionMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	fabricCushionMaterial.shininess = 0.2f;
	fabricCushionMaterial.tag = "fabric";

	m_objectMaterials.push_back(fabricCushionMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light 
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("lightSources[0].position", -60.0f, 50.0f, 30.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 100.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.01f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 30.0f, 30.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 60.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

}


void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderFloor();
	RenderBackWall();
	RenderRightWall();
	RenderLeftWall();
	RenderPicture();
	RenderFlowerPot();
	RenderTreeTrunk();
	RenderTreeLeaves();
	RenderCouchBase1();
	RenderCouchBase2();
	RenderCouchRightLeg();
	RenderCouchBack();
	RenderCouchLeftLeg();
	RenderCouchBackRightLeg();
	RenderCouchBackLeftLeg();
	RenderCouchLeftArm();
	RenderCouchRightArm();
	RenderCouchLeftCusion();
	RenderCouchRightCusion();
	RenderCouchLeftBackCusion();
	RenderCouchRightBackCusion();
	RenderCouchRightCusion1();
	RenderCouchRightCusion2();
	RenderCouchLeftCusion1();
	RenderCouchLeftCusion2();
	RenderCouchLeftCusion3();
	RenderTableTop();
	RenderTableleg1();
	RenderTableleg2();
	RenderTableleg3();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);     // wireframe mode
	/****************************************************************/
}



void SceneManager::RenderFloor()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	//Bottom Plane       a.k.a Floor
		/******************************************************************/
		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(40.0f, 1.0f, 25.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 20.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(174 / 255.0f, 159 / 255.0f, 138 / 255.0f, 255.0 / 255.0f);     // floor color
	SetShaderTexture("parquetTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("tile");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
}



void SceneManager::RenderBackWall()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	// Top Plane           a.k.a Back Wall
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(40.0f, 1.0f, 30.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 30.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(178 / 255.0f, 170 / 255.0f, 170 / 255.0f, 255.0 / 255.0f);     // wall color
	SetShaderTexture("drywallTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("tile");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
}




void SceneManager::RenderRightWall()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	// Right Plane           a.k.a Right Wall
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 1.0f, 30.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(40.0f, 30.0f, 20.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(178 / 255.0f, 170 / 255.0f, 170 / 255.0f, 255.0 / 255.0f);     // wall color
	SetShaderTexture("drywallTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("tile");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
}

void SceneManager::RenderLeftWall()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	// Left Plane           a.k.a Left Wall
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 1.0f, 30.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-40.0f, 30.0f, 20.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(178 / 255.0f, 170 / 255.0f, 170 / 255.0f, 255.0 / 255.0f);     // wall color
	SetShaderTexture("drywallTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
}



void SceneManager::RenderPicture()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Wall Cube           a.k.a Picture
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(18.0f, 18.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;   //-13
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 30.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);     // color
	SetShaderTexture("backdropTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}


void SceneManager::RenderFlowerPot()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// flower pot           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 7.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -5.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-25.0f, 7.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(163 / 255.0f, 139 / 255.0f, 103 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("wickerBasketTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/
}

void SceneManager::RenderTreeTrunk()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// tree trunk           
		/******************************************************************/
		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 20.0f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-25.0f, 1.0f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 60 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("rusticWoodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}




void SceneManager::RenderTreeLeaves()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	// tree leaves        
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 20.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-25.0f, 15.0f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(141 / 255.0f, 124 / 255.0f, 87 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("leavesTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
}



void SceneManager::RenderCouchBase1()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch Base Cube           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(34.5f, 0.65f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 66 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("rusticWoodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}



void SceneManager::RenderCouchBase2()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch Base2 Cube           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(30.0f, 1.25f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 2.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(90 / 255.0f, 70 / 255.0f, 50 / 255.0f, 250.0 / 255.0f);     // color
	SetShaderTexture("woodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}



void SceneManager::RenderCouchRightLeg()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch Base right leg           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.65f, 3.5f, 0.65f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 0.25f, 2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 66 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("rusticWoodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}




void SceneManager::RenderCouchBack()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch Base back Cube           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(34.0f, 1.00f, 7.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 7.0f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 70 / 255.0f, 50 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("rusticWoodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}




void SceneManager::RenderCouchLeftLeg()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch Base left leg           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.65f, 3.5f, 0.65f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 0.25f, 2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 60 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("rusticWoodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}




void SceneManager::RenderCouchBackRightLeg()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch Base back right leg           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.65f, 3.5f, 0.65f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 0.25f, -2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 60 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("rusticWoodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}




void SceneManager::RenderCouchBackLeftLeg()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch Base back left leg           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.65f, 3.5f, 0.65f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 0.25f, -2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 60 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("rusticWoodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}




void SceneManager::RenderCouchLeftArm()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch Base left arm Cube           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 6.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-16.75f, 6.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(90 / 255.0f, 70 / 255.0f, 50 / 255.0f, 250.0 / 255.0f);     // color
	SetShaderTexture("rusticWoodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}


void SceneManager::RenderCouchRightArm()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch Base right arm Cube           
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 6.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.75f, 6.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(90 / 255.0f, 70 / 255.0f, 50 / 255.0f, 250.0 / 255.0f);     // color
	SetShaderTexture("rusticWoodTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}




void SceneManager::RenderCouchLeftCusion()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch left cusion          
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(16.1f, 2.0f, 6.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.2f, 5.0f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);     // color
	SetShaderTexture("cottonTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}


void SceneManager::RenderCouchRightCusion()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch right cusion          
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(16.1f, 2.0f, 6.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.2f, 5.0f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//wSetShaderColor(1, 1, 1, 1);     // color
	SetShaderTexture("cottonTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}





void SceneManager::RenderCouchLeftBackCusion()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch left back cusion          
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(16.0f, 1.5f, 5.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 8.8f, -1.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);     // color
	SetShaderTexture("cotton2Texture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}



void SceneManager::RenderCouchRightBackCusion()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch right back cusion          
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(16.0f, 1.5f, 5.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.0f, 8.8f, -1.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);     // color
	SetShaderTexture("cotton2Texture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}






void SceneManager::RenderCouchRightCusion1()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch right cusion 1         
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 4.5f, .70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -15.0f;
	YrotationDegrees = -40.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 8.5f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(191 / 255.0f, 184 / 255.0f, 176 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("fabricTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}



void SceneManager::RenderCouchRightCusion2()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch right cusion 2       
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 4.0f, .70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -10.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.5f, 8.0f, 0.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(185 / 255.0f, 184 / 255.0f, 176 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("stainlessTexture");
	SetTextureUVScale(0.1, 1.0);
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}


void SceneManager::RenderCouchLeftCusion1()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch left cusion 1         
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 4.0f, .70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 50.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.2f, 8.8f, 0.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(191 / 255.0f, 184 / 255.0f, 176 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("stainlessTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}



void SceneManager::RenderCouchLeftCusion2()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch left cusion 2       
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 3.5f, .70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -15.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 5.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-14.5f, 7.7f, 2.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(191 / 255.0f, 184 / 255.0f, 176 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("stainlessTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}


void SceneManager::RenderCouchLeftCusion3()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Couch left cusion 3     
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 4.0f, .70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -10.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.0f, 7.7f, 1.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(185 / 255.0f, 184 / 255.0f, 176 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("fabricTexture");
	SetTextureUVScale(0.1, 0.5);
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}


void SceneManager::RenderTableTop()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// table top         
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 0.45f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 4.5f, 15.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 66 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("parquetTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("tile");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}


void SceneManager::RenderTableleg1()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// table top         
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 5.5f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -30.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.5f, 0.0f, 19.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 66 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("parquetTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("tile");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}


void SceneManager::RenderTableleg2()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// table top         
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 5.5f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = -30.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.0f, 0.0f, 15.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 66 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("parquetTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("tile");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}


void SceneManager::RenderTableleg3()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// table top         
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 5.5f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 20.0f;
	ZrotationDegrees = 30.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.0f, 15.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(100 / 255.0f, 66 / 255.0f, 45 / 255.0f, 255.0 / 255.0f);     // color
	SetShaderTexture("parquetTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("tile");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}