
#define _USE_MATH_DEFINES
#include <cmath>
#include <filesystem>
namespace fs = std::filesystem;

#include "Mesh.h"

const unsigned int width = 800;
const unsigned int height = 800;

// 
//  天体スケール定数
//  地球半径を基準単位 1.0 として、実際の直径比から月の半径を算出
//  地球半径: 6371km / 月半径: 1737.4km  ->  比率 約0.273
// 
const float EARTH_RADIUS = 1.0f;
const float MOON_RADIUS = EARTH_RADIUS * 0.273f;   // 実物の直径比

// 月の公転半径(実際は地球半径の約60.3倍 = 384,400km / 6,371km)
// そのままだと月が非常に小さく遠くに見えるため、見え方を確認しながら
// DISTANCE_SCALE で調整できるようにしている(1.0で完全な実比率)
const float DISTANCE_SCALE = 1.0f;
const float MOON_ORBIT_RADIUS = EARTH_RADIUS * 60.3f * DISTANCE_SCALE;

// 地軸の傾き(23.44度)、月の公転軌道傾斜(黄道に対して5.14度)
const float EARTH_AXIAL_TILT_DEG = 23.44f;
const float MOON_ORBIT_INCLINATION_DEG = 5.14f;


//  時間スケール(実時間だと遅すぎるので見た目重視で調整)
//  ただし「地球の自転27.32回で月が1公転する」という実際の比率は保持する

const float MOON_ORBIT_PERIOD_SEC = 60.0f;                         // 月が1周する秒数(お好みで調整可)
const float EARTH_ROTATION_PERIOD_SEC = MOON_ORBIT_PERIOD_SEC / 27.32f; // 実比率を保った地球の自転周期
const float MOON_ROTATION_PERIOD_SEC = MOON_ORBIT_PERIOD_SEC;       // 潮汐固定(月は常に同じ面を地球に向ける)

// 太陽(光源)の配置。実際の距離(地球半径の約23,455倍)は非現実的なので、
// 「地球から十分離れた場所」に置いて指向性光(平行光線)として扱う
const float SUN_DISTANCE = 200.0f;
const float SUN_VISUAL_RADIUS = 6.0f;   // 見た目用のサイズ(実際の太陽サイズとは無関係)

const float SKY_RADIUS = 300.0f; // 星空球のサイズ(SUN_DISTANCEより大きく、カメラのfarより小さく)



//  頂点データ生成ユーティリティ


// 球体(地球・月・太陽・星空球すべてで共用する半径1.0の単位球)
void CreateSphere(float radius, unsigned int sectorCount, unsigned int stackCount,
	std::vector<Vertex>& outVertices, std::vector<GLuint>& outIndices)
{
	outVertices.clear();
	outIndices.clear();

	float x, y, z, xy;
	float nx, ny, nz, lengthInv = 1.0f / radius;
	float s, t;

	float sectorStep = 2 * M_PI / sectorCount;
	float stackStep = M_PI / stackCount;
	float sectorAngle, stackAngle;

	for (unsigned int i = 0; i <= stackCount; ++i) {
		stackAngle = M_PI / 2 - i * stackStep;
		xy = radius * cosf(stackAngle);
		z = radius * sinf(stackAngle);

		for (unsigned int j = 0; j <= sectorCount; ++j) {
			sectorAngle = j * sectorStep;

			x = xy * cosf(sectorAngle);
			y = xy * sinf(sectorAngle);

			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;

			s = (float)j / sectorCount;
			t = (float)i / stackCount;

			Vertex vertex;
			vertex.position = glm::vec3(x, z, y);
			vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
			vertex.normal = glm::vec3(nx, nz, ny);
			vertex.texUV = glm::vec2(s, t);

			outVertices.push_back(vertex);
		}
	}

	unsigned int k1, k2;
	for (unsigned int i = 0; i < stackCount; ++i) {
		k1 = i * (sectorCount + 1);
		k2 = k1 + sectorCount + 1;

		for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
			if (i != 0) {
				outIndices.push_back(k1);
				outIndices.push_back(k2);
				outIndices.push_back(k1 + 1);
			}
			if (i != (stackCount - 1)) {
				outIndices.push_back(k1 + 1);
				outIndices.push_back(k2);
				outIndices.push_back(k2 + 1);
			}
		}
	}
}

// 月の公転軌道を示す円(XZ平面上、半径1.0の単位円)。GL_LINE_LOOPで描画する
void CreateOrbitRing(float radius, unsigned int segments,
	std::vector<Vertex>& outVertices, std::vector<GLuint>& outIndices)
{
	outVertices.clear();
	outIndices.clear();

	for (unsigned int i = 0; i < segments; ++i)
	{
		float angle = 2.0f * (float)M_PI * (float)i / (float)segments;

		Vertex v{};
		v.position = glm::vec3(cosf(angle) * radius, 0.0f, sinf(angle) * radius);
		v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
		v.color = glm::vec3(1.0f);
		v.texUV = glm::vec2(0.0f);

		outVertices.push_back(v);
		outIndices.push_back(i); // GL_LINE_LOOPなので連番のままでよい(最後は自動的に最初へ戻る)
	}
}

// シェーダーのmodel行列を更新するための小さなヘルパー
void SetModelUniform(Shader& shader, const glm::mat4& model)
{
	shader.Activate();
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
}

// light.vert/frag(無照明の単色シェーダー)用に色を設定するヘルパー
void SetFlatColorUniform(Shader& shader, const glm::vec4& color)
{
	shader.Activate();
	glUniform4f(glGetUniformLocation(shader.ID, "flatColor"), color.r, color.g, color.b, color.a);
}

// ウィンドウがリサイズされたときに呼ばれるコールバック。
// glViewportを更新しないと、ウィンドウを拡大しても描画範囲は元のサイズのままになり、
// 画面の一部にしか映像が表示されない(引き伸ばされる)状態になってしまう。
// あわせてCameraのwidth/heightも更新することで、アスペクト比とマウス操作の基準点も
// 新しいウィンドウサイズに追従する。
void framebuffer_size_callback(GLFWwindow* window, int newWidth, int newHeight)
{
	glViewport(0, 0, newWidth, newHeight);

	Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
	if (camera != nullptr)
	{
		camera->width = newWidth;
		camera->height = newHeight;
	}
}


int main()
{
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a GLFWwindow object of 800 by 800 pixels
	GLFWwindow* window = glfwCreateWindow(width, height, "Earth & Moon Simulation", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Load GLAD
	gladLoadGL();
	glViewport(0, 0, width, height);

	// Direct absolute directory configuration with double backslashes
	std::string baseDir = "C:\\Development\\OpenGLver2\\Resources\\pic\\";

	//  テクスチャ読み込み(ご指定の形式をそのまま踏襲)
	
	Texture earthTextures[]
	{
		Texture((baseDir + "earth.png").c_str(), "diffuse", 0, GL_RGBA, GL_UNSIGNED_BYTE),
		Texture((baseDir + "SkyBack.png").c_str(), "specular", 1, GL_RED, GL_UNSIGNED_BYTE)
	};

	Texture moonTextures[]
	{
		Texture((baseDir + "Moon.png").c_str(), "diffuse", 0, GL_RGBA, GL_UNSIGNED_BYTE),
		Texture((baseDir + "SkyBack.png").c_str(), "specular", 1, GL_RED, GL_UNSIGNED_BYTE)
	};

	Texture skyTextures[]
	{
		Texture((baseDir + "SkyBack.png").c_str(), "diffuse", 0, GL_RGBA, GL_UNSIGNED_BYTE)
	};

	std::vector<Texture> earthTex(earthTextures, earthTextures + sizeof(earthTextures) / sizeof(Texture));
	std::vector<Texture> moonTex(moonTextures, moonTextures + sizeof(moonTextures) / sizeof(Texture));
	std::vector<Texture> skyTex(skyTextures, skyTextures + sizeof(skyTextures) / sizeof(Texture));
	std::vector<Texture> noTex; // 太陽マーカー・軌道線は無テクスチャ(単色)

	// ============================================================
	//  シェーダー
	// ============================================================
	Shader shaderProgram("default.vert", "default.frag"); // 地球・月(太陽光を受けるライティング付き)
	Shader lightShader("light.vert", "light.frag");        // 太陽マーカー・軌道ハイライト線(単色・無照明)
	Shader skyShader("sky.vert", "sky.frag");               // 星空背景(単色テクスチャのみ・無照明)

	// ============================================================
	//  ジオメトリ生成
	//  地球・月・太陽・星空球はすべて「半径1.0の単位球」を使い回し、
	//  model行列のscaleだけでそれぞれのサイズを表現する
	// ============================================================
	std::vector<Vertex> sphereVerts;
	std::vector<GLuint> sphereInd;
	CreateSphere(1.0f, 48, 48, sphereVerts, sphereInd);

	Mesh earthMesh(sphereVerts, sphereInd, earthTex);
	Mesh moonMesh(sphereVerts, sphereInd, moonTex);
	Mesh sunMesh(sphereVerts, sphereInd, noTex);
	Mesh skyMesh(sphereVerts, sphereInd, skyTex);

	// 月の公転軌道ハイライト用の円(半径1.0)
	std::vector<Vertex> ringVerts;
	std::vector<GLuint> ringInd;
	CreateOrbitRing(1.0f, 128, ringVerts, ringInd);
	Mesh orbitRingMesh(ringVerts, ringInd, noTex, GL_LINE_LOOP);

	// ============================================================
	//  固定パラメータ(太陽の位置・色、月軌道の傾斜行列)
	// ============================================================
	glm::vec3 earthPos = glm::vec3(0.0f, 0.0f, 0.0f);

	// 太陽の方向を決めて、そこから十分遠い位置に配置する
	glm::vec3 sunDirection = glm::normalize(glm::vec3(1.0f, 0.35f, 0.6f));
	glm::vec3 sunPos = sunDirection * SUN_DISTANCE;

	glm::vec4 lightColor = glm::vec4(1.0f, 0.98f, 0.9f, 1.0f); // 太陽光の色(わずかに暖色)

	// 月の公転軌道面の傾き(黄道に対して5.14度)
	glm::mat4 orbitInclination = glm::rotate(glm::mat4(1.0f),
		glm::radians(MOON_ORBIT_INCLINATION_DEG), glm::vec3(1.0f, 0.0f, 0.0f));

	// ---- 太陽マーカー(静的。動かないので一度だけmodelを設定) ----
	glm::mat4 sunModel = glm::translate(glm::mat4(1.0f), sunPos);
	sunModel = glm::scale(sunModel, glm::vec3(SUN_VISUAL_RADIUS));

	// ---- 星空球(静的。カメラの可動域より十分大きい半径で包む) ----
	glm::mat4 skyModel = glm::scale(glm::mat4(1.0f), glm::vec3(SKY_RADIUS));

	// ---- 軌道ハイライト線(静的。地球中心・軌道傾斜・軌道半径) ----
	glm::mat4 ringModel = glm::translate(glm::mat4(1.0f), earthPos) * orbitInclination;
	ringModel = glm::scale(ringModel, glm::vec3(MOON_ORBIT_RADIUS));

	// ============================================================
	//  ライティング用ユニフォームの初期設定
	//  太陽は地球から十分遠いので、フラグメントシェーダー側では
	//  距離減衰のない「指向性光」として扱う(sunLight()関数を参照)
	// ============================================================
	shaderProgram.Activate();
	glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), sunPos.x, sunPos.y, sunPos.z);

	SetFlatColorUniform(lightShader, glm::vec4(1.0f, 0.95f, 0.8f, 1.0f)); // 太陽マーカーの色(このあと軌道線描画時に上書きする)

	SetModelUniform(skyShader, skyModel);

	// 深度テストを有効化
	glEnable(GL_DEPTH_TEST);

	// 軌道ハイライト線を半透明にするためブレンディングを有効化
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// カメラ(地球と月の軌道全体が見える位置からスタート)
	Camera camera(width, height, glm::vec3(0.0f, 25.0f, 130.0f));

	// ウィンドウリサイズに対応させるため、カメラへのポインタを紐付けてコールバックを登録する
	glfwSetWindowUserPointer(window, &camera);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// deltaTime計算用(フレームレートに依存しないカメラ移動のため)
	float lastFrameTime = 0.0f;

	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		// 宇宙空間らしい暗い背景色
		glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// カメラ操作・行列更新(farを軌道全体が収まるよう拡張)
		float currentFrameTime = (float)glfwGetTime();
		float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		camera.Inputs(window, deltaTime);
		camera.updateMatrix(45.0f, 0.1f, 500.0f);

		float t = currentFrameTime;

		// --------------------------------------------------------
		//  地球:自転(地軸を23.44度傾けた状態でY軸周りに自転)
		// --------------------------------------------------------
		float earthSpinDeg = 360.0f * t / EARTH_ROTATION_PERIOD_SEC;
		glm::mat4 earthModel = glm::translate(glm::mat4(1.0f), earthPos);
		earthModel = glm::rotate(earthModel, glm::radians(EARTH_AXIAL_TILT_DEG), glm::vec3(0.0f, 0.0f, 1.0f));
		earthModel = glm::rotate(earthModel, glm::radians(earthSpinDeg), glm::vec3(0.0f, 1.0f, 0.0f));
		earthModel = glm::scale(earthModel, glm::vec3(EARTH_RADIUS));

		// --------------------------------------------------------
		//  月:公転(地球の周りを軌道傾斜をつけて周回) + 自転(潮汐固定)
		// --------------------------------------------------------
		float moonOrbitDeg = 360.0f * t / MOON_ORBIT_PERIOD_SEC;
		float moonOrbitRad = glm::radians(moonOrbitDeg);

		glm::vec3 localOrbitPos = glm::vec3(cosf(moonOrbitRad) * MOON_ORBIT_RADIUS, 0.0f, sinf(moonOrbitRad) * MOON_ORBIT_RADIUS);
		glm::vec3 moonPos = earthPos + glm::vec3(orbitInclination * glm::vec4(localOrbitPos, 1.0f));

		glm::mat4 moonModel = glm::translate(glm::mat4(1.0f), moonPos);
		// 潮汐固定:自転角度を公転角度と一致させることで常に同じ面が地球を向く
		moonModel = glm::rotate(moonModel, moonOrbitRad, glm::vec3(0.0f, 1.0f, 0.0f));
		moonModel = glm::scale(moonModel, glm::vec3(MOON_RADIUS));

		
		skyMesh.Draw(skyShader, camera);

		SetFlatColorUniform(lightShader, glm::vec4(0.4f, 0.7f, 1.0f, 0.35f)); // 軌道線:水色でハイライト
		SetModelUniform(lightShader, ringModel);
		orbitRingMesh.Draw(lightShader, camera);

		SetModelUniform(shaderProgram, earthModel);
		earthMesh.Draw(shaderProgram, camera);

		SetModelUniform(shaderProgram, moonModel);
		moonMesh.Draw(shaderProgram, camera);

		SetFlatColorUniform(lightShader, glm::vec4(1.0f, 0.95f, 0.8f, 1.0f)); // 太陽マ明るい暖色
		SetModelUniform(lightShader, sunModel);
		sunMesh.Draw(lightShader, camera);

		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Delete all the objects has created
	shaderProgram.Delete();
	lightShader.Delete();
	skyShader.Delete();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
