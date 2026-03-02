///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include <iostream> 
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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "";
		m_textureIDs[i].ID = 0;
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
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

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
		std::cout << "Successfully loaded image:" << filename << ", width:" << width
			<< ", height:" << height << ", channels:" << colorChannels << std::endl;

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
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		}
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		}
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
		glDeleteTextures(1, &m_textureIDs[i].ID);
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
		{
			index++;
		}
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
		{
			index++;
		}
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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
void SceneManager::SetShaderColor(float redColorValue, float greenColorValue, float blueColorValue, float alphaValue)
{
	glm::vec4 currentColor(redColorValue, greenColorValue, blueColorValue, alphaValue);

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, 0);     // texture OFF
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
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/

void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	// Load cabinet wood texture for cabinet doors
	bReturn = CreateGLTexture("././textures/cabinet.jpg", "cabinet");

	// Load marble texture for the countertop surface
	bReturn = CreateGLTexture("././textures/marble.jpg", "marble");

	// Load general wood texture for cabinets in the scene
	bReturn = CreateGLTexture("././textures/wood.jpg", "wood");

	// Load metal texture for metallic objects (fridge)
	bReturn = CreateGLTexture("././textures/metal.jpg", "metal");

	// Load glossy black texture for pendant light material
	bReturn = CreateGLTexture("././textures/blackshiny.jpg", "blackshiny");

	// Load texture for apple
	bReturn = CreateGLTexture("././textures/apple.jpg", "apple");

	// Bind all loaded textures to OpenGL texture slots
	BindGLTextures();

}

void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	LoadSceneTextures(); 

	DefineObjectMaterials();   // Define Phong material properties for scene objects
	SetupSceneLights();       // Configure point lights used in the scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
}

void SceneManager::DefineObjectMaterials()
{
	// Matte black for pendant shade
	OBJECT_MATERIAL matteBlack;
	matteBlack.tag = "matteBlack";
	matteBlack.diffuseColor = glm::vec3(0.05f, 0.05f, 0.05f);
	matteBlack.specularColor = glm::vec3(0.10f, 0.10f, 0.10f);
	matteBlack.shininess = 8.0f;
	m_objectMaterials.push_back(matteBlack);

	// Slightly shiny black for the cord
	OBJECT_MATERIAL cordBlack;
	cordBlack.tag = "cordBlack";
	cordBlack.diffuseColor = glm::vec3(0.03f, 0.03f, 0.03f);
	cordBlack.specularColor = glm::vec3(0.20f, 0.20f, 0.20f);
	cordBlack.shininess = 24.0f;
	m_objectMaterials.push_back(cordBlack);

	// countertop should look a bit shiny
	OBJECT_MATERIAL marble;
	marble.tag = "marble";
	marble.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	marble.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	marble.shininess = 150.0f;
	m_objectMaterials.push_back(marble);

	// cabinets should be less shiny
	OBJECT_MATERIAL cabinet;
	cabinet.tag = "cabinet";
	cabinet.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	cabinet.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cabinet.shininess = 8.0f;
	m_objectMaterials.push_back(cabinet);

	// fridge metal = sharper highlights
	OBJECT_MATERIAL metal;
	metal.tag = "metal";
	metal.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	metal.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	metal.shininess = 96.0f;
	m_objectMaterials.push_back(metal);
	
	// wall/ceiling paint (matte)
	OBJECT_MATERIAL paint;
	paint.tag = "paint";
	paint.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	paint.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	paint.shininess = 8.0f;
	m_objectMaterials.push_back(paint);

	//
	OBJECT_MATERIAL white;
	white.tag = "white";
	white.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	white.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	white.shininess = 16.0f;
	m_objectMaterials.push_back(white);
}

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Light 0: Main ceiling light
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 18.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.18f, 0.18f, 0.18f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.65f, 0.65f, 0.65f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.30f, 0.30f, 0.30f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", 1);

	// Light 1: Soft fill light
	m_pShaderManager->setVec3Value("pointLights[1].position", -18.0f, 12.0f, 6.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.08f, 0.08f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.30f, 0.30f, 0.30f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.12f, 0.12f, 0.12f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", 1);

	// Light 2: Warm fruit light (SUBTLE so it doesn't tint the whole scene)
	m_pShaderManager->setVec3Value("pointLights[2].position", 8.5f, 4.1f, 4.2f); // near apples
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.01f, 0.008f, 0.004f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.18f, 0.12f, 0.06f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.08f, 0.06f, 0.03f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", 1);

	// Light 3: OFF
	m_pShaderManager->setBoolValue("pointLights[3].bActive", 0);
}


/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/

 /****************************************************************/
 /*                      TABLE TOP                               */
 /****************************************************************/
void SceneManager::RenderScene()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(15.0f, 2.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 20.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Ensure lighting is enabled so the countertop reacts to Phong shading
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Material controls specular highlight (Phong reflection)
	SetShaderMaterial("marble");

	// Texture on
	SetShaderTexture("marble");
	SetTextureUVScale(1.0f, 1.0f);

	// Countertop surface (marble texture + Phong material)
	m_basicMeshes->DrawPlaneMesh();


	/****************************************************************/
	/*                      ISLAND / COUNTER BOXES                  */
	/****************************************************************/

	// Turn lighting OFF + texture OFF for the next objects (use flat solid color, no Phong shading, no textures)
	m_pShaderManager->setBoolValue(g_UseLightingName, false);
	m_pShaderManager->setIntValue(g_UseTextureName, 0);

	// All boxes use the same scale
	const glm::vec3 boxScale = glm::vec3(6.0f, 5.0f, 2.5f);

	// Offset to move the whole island
	const glm::vec3 islandOffset = glm::vec3(3.0f, 0.0f, 2.0f); // change to move all boxes together

	// Base positions for each box (relative to island)
	const glm::vec3 boxPositions[5] = {
		glm::vec3(-15.0f, -0.5f, 1.8f),
		glm::vec3(-9.0f,  -0.5f, 1.8f),
		glm::vec3(-3.0f,  -0.5f, 1.8f),
		glm::vec3(3.0f,   -0.5f, 1.8f),
		glm::vec3(9.0f,   -0.5f, 1.8f)

	};

	m_pShaderManager->setIntValue(g_UseTextureName, 0);

	// Draw all boxes
	for (int i = 0; i < 5; ++i)
	{
		scaleXYZ = boxScale;
		positionXYZ = boxPositions[i] + islandOffset;

		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

		SetShaderMaterial("white");
		SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_basicMeshes->DrawBoxMesh();

		// Outline
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(1.0f);
		SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f);
		m_basicMeshes->DrawBoxMesh();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Turn lighting back ON so following objects react to scene lights (Phong shading active)
	m_pShaderManager->setBoolValue(g_UseLightingName, true);


	/****************************************************************/
	/*                      KITCHEN GROUP                           */
	/****************************************************************/

	// Kitchen offset
	const glm::vec3 kitchenOffset = glm::vec3(0.0f, 1.5f, 0.0f); // Move entire kitchen here

	// Cabinet offset
	const glm::vec3 cabinetOffset = glm::vec3(0.0f, 0.0f, -0.2f);

	// Colors
	const glm::vec4 cabinetBodyColor = glm::vec4(0.55f, 0.35f, 0.20f, 1.0f);
	const glm::vec4 cabinetDoorColor = glm::vec4(0.65f, 0.45f, 0.30f, 1.0f);
	const glm::vec4 fridgeBodyColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
	const glm::vec4 fridgePanelColor = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
	const glm::vec4 handleColor = glm::vec4(0.08f, 0.08f, 0.08f, 1.0f);
	const glm::vec4 dispenserColor = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
	const glm::vec4 dispenserRingColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);

	// Base cabinets
	const glm::vec3 baseCabinetPositions[] = {
		{ 12.0f, 9.0f, -10.0f },
		{  7.7f, 9.0f, -10.0f },
		{  3.4f, 9.0f, -10.0f },
		{ -0.9f, 9.0f, -10.0f },
		{ -5.8f, 9.0f, -10.0f }
	};

	const float baseCabinetWidths[] = { 4.5f, 4.5f, 4.5f, 4.5f, 5.5f };

	auto DrawCabinetTextured = [&](const glm::vec3& scale,
		const glm::vec3& pos,
		const std::string& texTag,
		float u, float v)
		{
			SetTransformations(scale, 0.0f, 0.0f, 0.0f, pos);

			// Lighting disabled for this object group to keep a flat, clean look
			m_pShaderManager->setBoolValue(g_UseLightingName, false);

			// Texture ON
			SetShaderTexture(texTag);
			SetTextureUVScale(u, v);

			m_basicMeshes->DrawBoxMesh();

			// Outline (no texture)
			m_pShaderManager->setIntValue(g_UseTextureName, 0);
			SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f);

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(1.0f);
			m_basicMeshes->DrawBoxMesh();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			// Restore lighting
			m_pShaderManager->setBoolValue(g_UseLightingName, true);
		};

	for (int i = 0; i < 5; ++i)
	{
		// Cabinet body (wood texture)
		scaleXYZ = glm::vec3(baseCabinetWidths[i], 18.0f, 1.0f);
		positionXYZ = baseCabinetPositions[i] + kitchenOffset + cabinetOffset;
		DrawCabinetTextured(scaleXYZ, positionXYZ, "wood", 2.0f, 2.0f);

		// Cabinet door (cabinet.jpg texture)
		scaleXYZ = glm::vec3(baseCabinetWidths[i] - 0.5f, 18.0f, 0.05f);
		glm::vec3 doorPos = positionXYZ;
		doorPos.z += 0.525f;
		DrawCabinetTextured(scaleXYZ, doorPos, "cabinet", 2.0f, 1.0f);
	}


	// Upper cabinets
	const glm::vec3 upperCabinetPositions[] = {
		{ -11.3f, 14.5f, -10.0f },
		{ -16.8f, 14.5f, -10.0f }
	};

	for (int i = 0; i < 2; ++i)
	{
		// Body
		scaleXYZ = glm::vec3(5.5f, 7.0f, 1.0f);
		positionXYZ = upperCabinetPositions[i] + kitchenOffset + cabinetOffset;
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

		// cabinet material so it reacts to light
		SetShaderMaterial("cabinet");
		// draw textured cabinet door
		SetShaderTexture("wood");
		// Tile the wood texture so the grain looks natural on the door surface
		SetTextureUVScale(2.0f, 2.0f);
		// Draw the cabinet door with the applied wood texture
		m_basicMeshes->DrawBoxMesh();

		// Body outline
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(1.0f);
		SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f);
		m_basicMeshes->DrawBoxMesh();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Doors
		scaleXYZ = glm::vec3(5.0f, 7.0f, 0.05f);
		positionXYZ.z += 0.525f;
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

		// cabinet material so it reacts to light
		SetShaderMaterial("cabinet");
		// Apply cabinet wood texture to the door surface
		SetShaderTexture("cabinet");
		// Tile the texture horizontally so the wood grain looks natural
		SetTextureUVScale(2.0f, 1.0f);
		// Draw the cabinet door with the texture applied
		m_basicMeshes->DrawBoxMesh();

		// Door outline (drawn in wireframe for edge definition)
		SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		m_basicMeshes->DrawBoxMesh();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(1.0f);
		SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f);
		m_basicMeshes->DrawBoxMesh();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Fridge
	scaleXYZ = glm::vec3(11.0f, 11.0f, 1.0f);
	positionXYZ = glm::vec3(-14.1f, 5.5f, -10.0f) + kitchenOffset;
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	//
	SetShaderMaterial("metal");
	// Apply metal texture to the object (fridge surface)
	SetShaderTexture("metal");
	// Use default UV scale so the metal texture fits normally
	SetTextureUVScale(1.0f, 1.0f);
	// Draw the mesh with the metal texture applied
	m_basicMeshes->DrawBoxMesh();

	// Fridge panel
	scaleXYZ = glm::vec3(2.5f, 3.5f, 0.05f);
	positionXYZ = glm::vec3(-10.8f, 4.0f, -9.475f) + kitchenOffset;
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(fridgePanelColor.r, fridgePanelColor.g, fridgePanelColor.b, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Cabinet handles
	scaleXYZ = glm::vec3(0.1f, 3.5f, 0.1f);
	SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);

	const glm::vec3 fridgeHandlePositions[] = {
		{  9.2f, 2.5f, -9.25f },
		{  1.8f, 2.5f, -9.25f },
		{ -3.7f, 2.5f, -9.25f }
	};

	for (int i = 0; i < 3; ++i)
	{
		positionXYZ = fridgeHandlePositions[i] + kitchenOffset;
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		m_basicMeshes->DrawCylinderMesh();
	}

	// Water dispenser
	scaleXYZ = glm::vec3(0.3f, 0.6f, 0.2f);
	positionXYZ = glm::vec3(-10.5f, 3.5f, -9.3f) + kitchenOffset;
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(dispenserColor.r, dispenserColor.g, dispenserColor.b, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.3f, 0.6f, 0.15f);
	positionXYZ = glm::vec3(-10.5f, 4.0f, -9.3f) + kitchenOffset;
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(dispenserRingColor.r, dispenserRingColor.g, dispenserRingColor.b, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	// Wall – Back
	/****************************************************************/
	scaleXYZ = glm::vec3(70.0f, 25.0f, 1.0f);  // Length, height, thickness
	positionXYZ = glm::vec3(0.0f, 10.0f, -15.0f); // centered, Y = height/2
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("paint");
	SetShaderColor(0.6f, 0.8f, 1.0f, 1.0f);   // Light blue wall
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	// Ceiling
	/****************************************************************/
	// Lighting OFF + Texture OFF (flat color, no shading/texture)
	m_pShaderManager->setBoolValue(g_UseLightingName, false);
	m_pShaderManager->setIntValue(g_UseTextureName, 0);

	// Ceiling
	scaleXYZ = glm::vec3(60.0f, 1.0f, 20.0f);
	positionXYZ = glm::vec3(0.0f, 20.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	SetShaderMaterial("white");
	SetShaderColor(0.95f, 0.95f, 0.95f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// Lighting ON for rest of scene
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Pendant Lights
	const glm::vec3 lightPos[3] = {
		glm::vec3(-10.0f, 18.5f, 2.0f), // left
		glm::vec3(0.0f, 18.5f, 2.0f), // center
		glm::vec3(10.0f, 18.5f, 2.0f) // right
	};

	for (int i = 0; i < 3; i++)
	{
		// Disable textures for cord/stem (solid color only)
		m_pShaderManager->setIntValue(g_UseTextureName, 0);

		// Hanging cord
		scaleXYZ = glm::vec3(0.04f, 1.5f, 0.06f);
		positionXYZ = lightPos[i] + glm::vec3(0.0f, -2.5f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderMaterial("cordBlack");
		SetTextureUVScale(1.0f, 6.0f);
		SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Ceiling stem
		scaleXYZ = glm::vec3(0.12f, 1.5f, 0.12f);
		positionXYZ = lightPos[i] + glm::vec3(0.0f, -4.0f, 0.0f);
		SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
		SetShaderMaterial("cordBlack");
		SetTextureUVScale(1.0f, 2.0f);
		SetShaderColor(0.02f, 0.02f, 0.02f, 1);
		m_basicMeshes->DrawCylinderMesh();

		// Lamp shade (cone)
		scaleXYZ = glm::vec3(1.0f, 2.5f, 1.0f);
		positionXYZ = lightPos[i] + glm::vec3(0.0f, -5.5f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderMaterial("matteBlack");
		SetTextureUVScale(1.0f, 2.0f);
		m_basicMeshes->DrawConeMesh();

		// Glowing bulb (emissive look)
		m_pShaderManager->setBoolValue(g_UseLightingName, false);

		scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);
		positionXYZ = lightPos[i] + glm::vec3(0.0f, -5.6f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderColor(1.0f, 0.98f, 0.80f, 0.75f);
		m_basicMeshes->DrawSphereMesh();

		// Re-enable lighting for rest of scene
		m_pShaderManager->setBoolValue(g_UseLightingName, true);
	}


	/****************************************************************/
	/*                      FAUCET + SINK (SINK TEXTURE)            */
	/****************************************************************/

	auto UseSinkTexture = [&]()
		{
			m_pShaderManager->setBoolValue(g_UseLightingName, true);

			// Keep texture ON
			m_pShaderManager->setIntValue(g_UseTextureName, 1);

			// Set objectColor to white without calling SetShaderColor() (keeps bUseTexture ON)
			m_pShaderManager->setVec4Value(g_ColorValueName, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

			SetShaderMaterial("metal");
			SetShaderTexture("sink");
			SetTextureUVScale(1.0f, 1.0f);
		};


	/****************************************************************/
	/*                          FAUCET                              */
	/****************************************************************/

	glm::vec3 faucetBase = glm::vec3(0.0f, 3.3f, 2.0f);

	UseSinkTexture();

	// (1) Base plate
	scaleXYZ = glm::vec3(0.40f, 0.05f, 0.40f);
	positionXYZ = faucetBase;
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	// (2) Vertical body
	UseSinkTexture();
	scaleXYZ = glm::vec3(0.2f, 2.3f, 0.2f);
	positionXYZ = faucetBase + glm::vec3(0.0f, 0.85f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	// (3) Head block
	UseSinkTexture();
	scaleXYZ = glm::vec3(0.34f, 0.40f, 0.28f);
	positionXYZ = faucetBase + glm::vec3(0.0f, 1.65f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// (4) Spout
	UseSinkTexture();
	scaleXYZ = glm::vec3(0.10f, 0.10f, 0.55f);
	positionXYZ = faucetBase + glm::vec3(0.0f, 1.60f, 0.45f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	// (5) Nozzle tip (still sink texture)
	UseSinkTexture();
	scaleXYZ = glm::vec3(0.11f, 0.03f, 0.11f);
	positionXYZ = faucetBase + glm::vec3(0.0f, 1.60f, 0.75f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	// (6) Lever handle
	UseSinkTexture();
	scaleXYZ = glm::vec3(0.08f, 0.26f, 0.52f);
	positionXYZ = faucetBase + glm::vec3(0.0f, 1.90f, -0.05f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 12.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();


	/****************************************************************/
	/*                            SINK                              */
	/****************************************************************/

	glm::vec3 sinkCenter = glm::vec3(0.0f, 3.85f, 3.3f);

	// (1) Outer sink rim
	UseSinkTexture();
	scaleXYZ = glm::vec3(4.0f, 0.18f, 2.0f);
	positionXYZ = sinkCenter + glm::vec3(0.0f, 0.10f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// (2) Inner basin
	UseSinkTexture();
	scaleXYZ = glm::vec3(4.0f, 0.95f, 1.3f);
	positionXYZ = sinkCenter + glm::vec3(0.0f, -0.55f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// (3) Drain (sink texture too)
	UseSinkTexture();
	scaleXYZ = glm::vec3(0.20f, 0.03f, 0.20f);
	positionXYZ = sinkCenter + glm::vec3(0.0f, -1.05f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*                          APPLES                              */
	/****************************************************************/
	// Fruit group anchor position (move this to reposition the apples and bowl together)
	glm::vec3 fruitCenter = glm::vec3(8.5f, 3.8f, 4.2f); // X, Y, Z

	// Use lighting so they look round/shaded
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Glass bowl
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	m_pShaderManager->setIntValue(g_UseTextureName, 0);

	// glass should have highlights
	SetShaderMaterial("metal");

	// light gray/blue tint + alpha transparency
	SetShaderColor(0.85f, 0.90f, 0.95f, 0.35f);  // last number = alpha (0.0-1.0)

	scaleXYZ = glm::vec3(2.0f, 0.15f, 1.2f);
	positionXYZ = fruitCenter + glm::vec3(0.0f, -0.50f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	/************ Apples (TEXTURE ON) ************/
	struct Apple { glm::vec3 offset; float size; };

	Apple apples[] =
	{
		// Front row (wide)
		{ glm::vec3(-0.55f, 0.00f,  0.30f), 0.35f },
		{ glm::vec3(0.55f, 0.02f,  0.25f), 0.33f },

		// Left / right middle
		{ glm::vec3(-0.60f, 0.05f, -0.20f), 0.34f },
		{ glm::vec3(0.60f, 0.05f, -0.15f), 0.36f },

		// Center apples
		{ glm::vec3(-0.15f, 0.10f,  0.00f), 0.34f },
		{ glm::vec3(0.20f, 0.12f,  0.10f), 0.32f },

		// Back row (spread)
		{ glm::vec3(-0.40f, 0.18f, -0.45f), 0.33f },
		{ glm::vec3(0.40f, 0.20f, -0.50f), 0.34f }
	};

	// Turn texture ON and bind apple texture
	m_pShaderManager->setIntValue(g_UseTextureName, 1);
	SetShaderMaterial("metal");          // keeps them a bit shiny
	SetShaderTexture("apple");
	SetTextureUVScale(1.0f, 1.0f);

	for (auto& a : apples)
	{
		scaleXYZ = glm::vec3(a.size, a.size, a.size);
		positionXYZ = fruitCenter + a.offset;
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		m_basicMeshes->DrawSphereMesh();
	}

	/************ Stems (brown, NO texture) ************/
	m_pShaderManager->setIntValue(g_UseTextureName, 0);
	SetShaderMaterial("matteBlack");
	SetShaderColor(0.12f, 0.07f, 0.02f, 1.0f);

	for (auto& a : apples)
	{
		scaleXYZ = glm::vec3(0.03f, 0.10f, 0.03f);
		positionXYZ = fruitCenter + a.offset + glm::vec3(0.0f, a.size + 0.05f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		m_basicMeshes->DrawCylinderMesh();
	}

}