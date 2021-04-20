#include <imgui.h>
#include <random>

#include "Application.h"
#include "Logger.h"
#include "FirsrPersonCamera.h"
#include "ThirdPersonCamera.h"
#include "Shader.h"
#include "MatrixStack.h"
#include "Texture2D.h"
#include "Light.h"
#include "Fog.h"

#include "Rectangle.h"
#include "Cube.h"
#include "Sphere.h"
#include "ViewVolume.h"

#include "Ball.h"
#include "Obstacle.h"

std::mt19937_64 rand_generator;
std::uniform_real_distribution<float> unif_ball_position_xz(-8, 8);
std::uniform_real_distribution<float> unif_ball_position_y(2, 18);
std::uniform_real_distribution<float> unif_ball_velocity(-2, 2);

class NexusDemo final : public Nexus::Application {
public:
	NexusDemo() {
		Settings.Width = 800;
		Settings.Height = 600;
		Settings.WindowTitle = "Game Engine #2 | Collision Detection & Culling";
		Settings.EnableDebugCallback = true;
		Settings.EnableFullScreen = false;

		Settings.EnableGhostMode = true;
		Settings.EnableBackFaceCulling = true;
		Settings.ShowOriginAnd3Axes = false;
		
		Settings.UseBlinnPhongShading = true;
		Settings.UseLighting = true;
		Settings.UseDiffuseTexture = true;
		Settings.UseSpecularTexture = true;
		Settings.UseEmission = true;
		Settings.UseGamma = true;
		Settings.GammaValue = 1.143f;

		// Projection Settings Initalize
		ProjectionSettings.IsPerspective = true;
		ProjectionSettings.OrthogonalHeight = 5.0f;
		ProjectionSettings.ClippingNear = 0.1f;
		ProjectionSettings.ClippingFar = 500.0f;
		ProjectionSettings.Aspect = (float)Settings.Width / (float)Settings.Height;
	}

	void Initialize() override {

		// Setting OpenGL
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Create shader program
		myShader = std::make_unique<Nexus::Shader>("Shaders/lighting.vert", "Shaders/lighting.frag");
		normalShader = std::make_unique<Nexus::Shader>("Shaders/normal_visualization.vs", "Shaders/normal_visualization.fs", "Shaders/normal_visualization.gs");
		
		// Create Camera
		first_camera = std::make_unique<Nexus::FirstPersonCamera>(glm::vec3(0.0f, 2.0f, 5.0f));
		third_camera = std::make_unique<Nexus::ThirdPersonCamera>(glm::vec3(0.0f, 0.0f, 5.0f));
		first_camera->SetRestrict(true);
		first_camera->SetRestrictValue(glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(10.0f, 20.0f, 10.0f));
		third_camera->SetRestrict(true);
		third_camera->SetRestrictValue(glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(10.0f, 20.0f, 10.0f));
		
		// Create Matrix Stack
		model = std::make_unique<Nexus::MatrixStack>();

		// Create object data
		floor = std::make_unique<Nexus::Rectangle>(20.0f, 20.0f, 10.0f, Nexus::POS_Y);
		cube = std::make_unique<Nexus::Cube>();
		sphere = std::make_unique<Nexus::Sphere>();

		view_volume = std::make_unique<Nexus::ViewVolume>();

		// Loading textures
		texture_checkerboard = Nexus::Texture2D::CreateFromFile("Resource/Textures/chessboard-metal.png", true);
		texture_checkerboard->SetWrappingParams(GL_REPEAT, GL_REPEAT);

		// Initial Light Setting
		DirLights = {
			new Nexus::DirectionalLight(glm::vec3(-0.2f, -1.0f, -0.3f), true)
		};
		PointLights = {
			new Nexus::PointLight(glm::vec3(-7.1f, 14.2f, -1.4f), false),
			new Nexus::PointLight(glm::vec3(7.1f, 17.1f, 1.4f), false),
			new Nexus::PointLight(glm::vec3(-5.7f, 4.2f, -5.7f), false),
			new Nexus::PointLight(glm::vec3(2.0f, 25.0f, 2.0f), false),
		};
		SpotLights = {
			new Nexus::SpotLight(third_camera->GetPosition(), third_camera->GetFront(), false),
			new Nexus::SpotLight(first_camera->GetPosition(), first_camera->GetFront(), false)
		};
		SpotLights[0]->SetCutoff(10.0f);
		SpotLights[0]->SetOuterCutoff(25.0f);
		SpotLights[1]->SetCutoff(6.0f);
		SpotLights[1]->SetOuterCutoff(30.0f);

		// Fog
		fog = std::make_unique<Nexus::Fog>(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), true, 0.1f, 100.0f);
		fog->SetDensity(0.01f);

		// Balls
		for (unsigned int i = 0; i < 200; i++) {
			glm::vec3 ball_position = glm::vec3(unif_ball_position_xz(rand_generator), unif_ball_position_y(rand_generator), unif_ball_position_xz(rand_generator));
			glm::vec3 ball_velocity = glm::vec3(unif_ball_velocity(rand_generator), unif_ball_velocity(rand_generator), unif_ball_velocity(rand_generator));
			Ball temp_ball(ball_position, ball_velocity);
			balls.push_back(temp_ball);
		}

		// Obstacle
		obstacles = {
			Obstacle(glm::vec3(3.0, 1.01f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
			Obstacle(glm::vec3(-3.0, 8.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
			Obstacle(glm::vec3(-3.0, 5.0f, -3.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
			Obstacle(glm::vec3(3.0, 15.0f, -3.0f), glm::vec3(0.0f, 0.0f, 0.0f))
		};
	}
	
	void Update(Nexus::DisplayMode monitor_type) override {

		if(Settings.EnableBackFaceCulling) {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CW);
		} else {
			glDisable(GL_CULL_FACE);
		}

		SetViewMatrix(monitor_type);
		SetProjectionMatrix(monitor_type);
		SetViewport(monitor_type);

		myShader->Use();
		myShader->SetBool("enableCulling", false);
		myShader->SetInt("material.diffuse_texture", 0);
		myShader->SetInt("material.specular_texture", 1);
		myShader->SetInt("material.emission_texture", 2);
		myShader->SetInt("skybox", 3);

		myShader->SetMat4("view", view);
		myShader->SetMat4("projection", projection);
		myShader->SetVec3("viewPos", Settings.EnableGhostMode ? first_camera->GetPosition() : third_camera->GetPosition());

		myShader->SetBool("useBlinnPhong", Settings.UseBlinnPhongShading);
		myShader->SetBool("useLighting", Settings.UseLighting);
		myShader->SetBool("useDiffuseTexture", Settings.UseDiffuseTexture);
		myShader->SetBool("useSpecularTexture", Settings.UseSpecularTexture);
		myShader->SetBool("useEmission", Settings.UseEmission);
		myShader->SetBool("useGamma", Settings.UseGamma);
		myShader->SetFloat("GammaValue", Settings.GammaValue);
		
		for (unsigned int i = 0; i < DirLights.size(); i++) {
			myShader->SetVec3("lights[" + std::to_string(i) + "].direction", DirLights[i]->GetDirection());
			myShader->SetVec3("lights[" + std::to_string(i) + "].ambient", DirLights[i]->GetAmbient());
			myShader->SetVec3("lights[" + std::to_string(i) + "].diffuse", DirLights[i]->GetDiffuse());
			myShader->SetVec3("lights[" + std::to_string(i) + "].specular", DirLights[i]->GetSpecular());
			myShader->SetBool("lights[" + std::to_string(i) + "].enable", DirLights[i]->GetEnable());
			myShader->SetInt("lights[" + std::to_string(i) + "].caster", DirLights[i]->GetCaster());
		}

		for (unsigned int i = 0; i < PointLights.size(); i++) {
			myShader->SetVec3("lights[" + std::to_string(i + 1) + "].position", PointLights[i]->GetPosition());
			myShader->SetVec3("lights[" + std::to_string(i + 1) + "].ambient", PointLights[i]->GetAmbient());
			myShader->SetVec3("lights[" + std::to_string(i + 1) + "].diffuse", PointLights[i]->GetDiffuse());
			myShader->SetVec3("lights[" + std::to_string(i + 1) + "].specular", PointLights[i]->GetSpecular());
			myShader->SetFloat("lights[" + std::to_string(i + 1) + "].constant", PointLights[i]->GetConstant());
			myShader->SetFloat("lights[" + std::to_string(i + 1) + "].linear", PointLights[i]->GetLinear());
			myShader->SetFloat("lights[" + std::to_string(i + 1) + "].quadratic", PointLights[i]->GetQuadratic());
			myShader->SetFloat("lights[" + std::to_string(i + 1) + "].enable", PointLights[i]->GetEnable());
			myShader->SetInt("lights[" + std::to_string(i + 1) + "].caster", PointLights[i]->GetCaster());
		}

		SpotLights[0]->SetPosition(third_camera->GetPosition());
		SpotLights[0]->SetDirection(third_camera->GetFront());
		SpotLights[1]->SetPosition(first_camera->GetPosition());
		SpotLights[1]->SetDirection(first_camera->GetFront());
		for (unsigned int i = 0; i < SpotLights.size(); i++) {
			myShader->SetVec3("lights[" + std::to_string(i + 5) + "].position", SpotLights[i]->GetPosition());
			myShader->SetVec3("lights[" + std::to_string(i + 5) + "].direction", SpotLights[i]->GetDirection());
			myShader->SetVec3("lights[" + std::to_string(i + 5) + "].ambient", SpotLights[i]->GetAmbient());
			myShader->SetVec3("lights[" + std::to_string(i + 5) + "].diffuse", SpotLights[i]->GetDiffuse());
			myShader->SetVec3("lights[" + std::to_string(i + 5) + "].specular", SpotLights[i]->GetSpecular());
			myShader->SetFloat("lights[" + std::to_string(i + 5) + "].constant", SpotLights[i]->GetConstant());
			myShader->SetFloat("lights[" + std::to_string(i + 5) + "].linear", SpotLights[i]->GetLinear());
			myShader->SetFloat("lights[" + std::to_string(i + 5) + "].quadratic", SpotLights[i]->GetQuadratic());
			myShader->SetFloat("lights[" + std::to_string(i + 5) + "].cutoff", glm::cos(glm::radians(SpotLights[i]->GetCutoff())));
			myShader->SetFloat("lights[" + std::to_string(i + 5) + "].outerCutoff", glm::cos(glm::radians(SpotLights[i]->GetOuterCutoff())));
			myShader->SetBool("lights[" + std::to_string(i + 5) + "].enable", SpotLights[i]->GetEnable());
			myShader->SetInt("lights[" + std::to_string(i + 5) + "].caster", SpotLights[i]->GetCaster());
		}

		myShader->SetVec4("fog.color", fog->GetColor());
		myShader->SetFloat("fog.density", fog->GetDensity());
		myShader->SetInt("fog.mode", fog->GetMode());
		myShader->SetInt("fog.depthType", fog->GetDepthType());
		myShader->SetBool("fog.enable", fog->GetEnable());
		myShader->SetFloat("fog.f_start", fog->GetFogStart());
		myShader->SetFloat("fog.f_end", fog->GetFogEnd());

		for (unsigned int i = 0; i < 6; i++) {
			if (i <= 2) {
				myShader->SetVec3("clippingPlanes[" + std::to_string(i) + "].position", view_volume->NearPlaneVertex[0]);
			} else {
				myShader->SetVec3("clippingPlanes[" + std::to_string(i) + "].position", view_volume->FarPlaneVertex[1]);
			}
			myShader->SetVec3("clippingPlanes[" + std::to_string(i) + "].normal", view_volume->ViewVolumeNormal[i]);
		}
		
		// ==================== Draw origin and 3 axes ====================
		if (Settings.ShowOriginAnd3Axes) {
			this->DrawOriginAnd3Axes(myShader.get());
		}

		// ==================== Draw a room ====================
		myShader->SetBool("material.enableDiffuseTexture", true);
		myShader->SetBool("material.enableSpecularTexture", false);
		myShader->SetBool("material.enableEmission", false);
		myShader->SetBool("material.enableEmissionTexture", false);
		myShader->SetFloat("material.shininess", 64.0f);
		model->Push();
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 20.0f, 0.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 10.0f, -10.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 10.0f, 10.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();
		
		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(10.0f, 10.0f, 0.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(-10.0f, 10.0f, 0.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();
		model->Pop();
		
		// ==================== Draw Ball ====================
		myShader->SetBool("enableCulling", enalbe_ball_culling);
		myShader->SetBool("material.enableDiffuseTexture", false);
		myShader->SetBool("material.enableSpecularTexture", false);
		myShader->SetBool("material.enableEmission", false);
		myShader->SetBool("material.enableEmissionTexture", false);
		myShader->SetFloat("material.shininess", 32.0f);
		for (unsigned int i = 0; i < balls.size(); i++) {
			myShader->SetVec4("material.ambient", balls[i].GetAmbient());
			myShader->SetVec4("material.diffuse", balls[i].GetDiffuse());
			myShader->SetVec4("material.specular", balls[i].GetSpecular());
			balls[i].ViewVolumeIncludingTest(view_volume.get());
			balls[i].Edge();
			for(unsigned int j = (i + 1); j < balls.size(); j++) {
				balls[i].CollisionWithBall(balls[j]);
			}
			
			for(unsigned int j = 0; j < obstacles.size(); j++) {
				balls[i].CollisionWithObstacle(obstacles[j]);
			}
			balls[i].Update(DeltaTime);
			
			model->Push();
			model->Save(balls[i].GetModel());
			sphere->Draw(myShader.get(), model->Top());
			model->Pop();
		}

		// ==================== Draw a cube ====================
		myShader->SetBool("enableCulling", false);
		myShader->SetBool("material.enableDiffuseTexture", false);
		myShader->SetBool("material.enableSpecularTexture", false);
		myShader->SetBool("material.enableEmission", false);
		myShader->SetBool("material.enableEmissionTexture", false);
		myShader->SetVec4("material.ambient", glm::vec4(0.02f, 0.02f, 0.02f, 1.0));
		myShader->SetVec4("material.diffuse", glm::vec4(0.1f, 0.35f, 0.1f, 1.0));
		myShader->SetVec4("material.specular", glm::vec4(0.45f, 0.55f, 0.45f, 1.0));
		myShader->SetFloat("material.shininess", 16.0f);
		for (unsigned int i = 0; i < obstacles.size(); i++) {

			// obstacles[i].Edge();
			// obstacles[i].Update(DeltaTime);

			model->Push();
			model->Save(glm::translate(model->Top(), obstacles[i].GetPosition()));
			model->Save(glm::scale(model->Top(), glm::vec3(2.0f)));
			cube->Draw(myShader.get(), model->Top());
			model->Pop();
		}

		// ==================== Draw View Volume ====================
		SetViewMatrix(Nexus::DISPLAY_MODE_DEFAULT);
		view_volume->UpdateVertices(
			Settings.EnableGhostMode ? first_camera->GetFOV() : third_camera->GetFOV(), 
			ProjectionSettings.IsPerspective,
			Settings.Width,
			Settings.Height,
			ProjectionSettings.ClippingNear,
			ProjectionSettings.ClippingFar,
			ProjectionSettings.OrthogonalHeight,
			view
		);
		model->Push();
		myShader->SetVec4("material.ambient", glm::vec4(0.2f, 0.2f, 0.2f, 0.6f));
		myShader->SetVec4("material.diffuse", glm::vec4(0.6f, 0.6f, 0.6f, 0.6f));
		myShader->SetVec4("material.specular", glm::vec4(0.0f, 0.0, 0.0, 0.6f));
		myShader->SetFloat("material.shininess", 32.0f);
		myShader->SetMat4("model", model->Top());
		view_volume->Draw(myShader.get(), model->Top());
		
		model->Pop();
		third_camera->SetTarget(balls[0].GetPosition());
		myShader->Use();
		
		// ==================== Draw Light Balls ====================
		myShader->SetBool("material.enableDiffuseTexture", false);
		myShader->SetBool("material.enableSpecularTexture", false);
		myShader->SetBool("material.enableEmission", true);
		myShader->SetBool("material.enableEmissionTexture", false);
		for(unsigned int i = 0; i < PointLights.size(); i++) {
			if (!PointLights[i]->GetEnable()) {
				continue;
			}
			model->Push();
				model->Save(glm::translate(model->Top(), PointLights[i]->GetPosition()));
				model->Save(glm::scale(model->Top(), glm::vec3(0.5f)));
				myShader->SetVec4("material.ambient", glm::vec4(PointLights[i]->GetAmbient(), 1.0f));
				myShader->SetVec4("material.diffuse", glm::vec4(PointLights[i]->GetDiffuse(), 1.0f));
				myShader->SetVec4("material.specular", glm::vec4(PointLights[i]->GetSpecular(), 1.0f));
				myShader->SetFloat("material.shininess", 32.0f);
				sphere->Draw(myShader.get(), model->Top());
			model->Pop();
		}
		myShader->SetBool("material.enableEmission", false);

		// ImGui::ShowDemoWindow();
	}

	void ShowDebugUI() override {
		ImGui::Begin("Control Panel");
		ImGuiTabBarFlags tab_bar_flags = ImGuiBackendFlags_None;

		if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {

			if (ImGui::BeginTabItem("Camera")) {
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				if (Settings.EnableGhostMode) {
					first_camera->ShowDebugUI("Person Person Camera");
				} else {
					third_camera->ShowDebugUI("Third Person Camera");
					ImGui::BulletText("Distance: %.2f", third_camera->GetDistance());
				}
				if (ImGui::Button("Reset Position")) {
					first_camera->SetPosition(glm::vec3(0.0f, 3.0f, 5.0f));
					first_camera->SetPitch(0.0f);
					first_camera->SetYaw(0.0f);
				}
				ImGui::EndTabItem();
			}
			

			if (ImGui::BeginTabItem("Ball")) {
				ImGui::Text("Ball amount: %d", balls.size());
				if (ImGui::Button("Add")) {
					Ball temp_ball(first_camera->GetPosition(), first_camera->GetFront() * abs(unif_ball_velocity(rand_generator)));
					balls.push_back(temp_ball);
				}
				if (ImGui::Button("Add 10")) {
					for (unsigned int i = 0; i < 10; i++) {
						glm::vec3 ball_position = glm::vec3(unif_ball_position_xz(rand_generator), unif_ball_position_y(rand_generator), unif_ball_position_xz(rand_generator));
						glm::vec3 ball_velocity = glm::vec3(unif_ball_velocity(rand_generator), unif_ball_velocity(rand_generator), unif_ball_velocity(rand_generator));
						Ball temp_ball(ball_position, ball_velocity);
						balls.push_back(temp_ball);
					}
				}
				if(!balls.empty()) {
					if (ImGui::Button("Delete")) {
						balls.pop_back();
					}
					if(balls.size() >= 10) {
						if (ImGui::Button("Delete 10")) {
							for (unsigned int i = 0; i < 10; i++) {
								balls.pop_back();
							}
						}
					}
					if (ImGui::Button("Delete All")) {
						balls.clear();
					}
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Projection")) {

				ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), (ProjectionSettings.IsPerspective) ? "Perspective Projection" : "Orthogonal Projection");
				ImGui::Text("Parameters");
				ImGui::BulletText("FoV = %.2f deg, Aspect = %.2f", Settings.EnableGhostMode ? first_camera->GetFOV() : third_camera->GetFOV(), ProjectionSettings.Aspect);
				if (!ProjectionSettings.IsPerspective) {
					ImGui::SliderFloat("Length", &ProjectionSettings.OrthogonalHeight, 1.0f, 100.0f);
				}
				ImGui::BulletText("left: %.2f, right: %.2f ", view_volume->ClippingParameters[2], view_volume->ClippingParameters[3]);
				ImGui::BulletText("bottom: %.2f, top: %.2f ", view_volume->ClippingParameters[0], view_volume->ClippingParameters[1]);
				ImGui::DragFloatRange2("Near & Far", &ProjectionSettings.ClippingNear, &ProjectionSettings.ClippingFar, 0.1f, 0.1f, 500.0f);
				ImGui::Spacing();

				if (ImGui::TreeNode("Projection Matrix")) {
					SetProjectionMatrix(Nexus::DISPLAY_MODE_DEFAULT);
					glm::mat4 proj = projection;

					ImGui::Columns(4, "mycolumns");
					ImGui::Separator();
					for (int i = 0; i < 4; i++) {
						ImGui::Text("%.2f", proj[0][i]); ImGui::NextColumn();
						ImGui::Text("%.2f", proj[1][i]); ImGui::NextColumn();
						ImGui::Text("%.2f", proj[2][i]); ImGui::NextColumn();
						ImGui::Text("%.2f", proj[3][i]); ImGui::NextColumn();
						ImGui::Separator();
						
					}
					ImGui::Columns(1);

					ImGui::TreePop();
				}
				ImGui::Spacing();
				
				if (ImGui::TreeNode("View Volume Vertices")) {
					view_volume->ShowViewVolumeVerticesImGUI();
					ImGui::TreePop();
				}
				ImGui::Spacing();

				if (ImGui::TreeNode("View Volume Normals")) {
					view_volume->ShowViewVolumeNormalsImGUI();
					ImGui::TreePop();
				}
				ImGui::Spacing();

				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Lightning")) {

				ImGui::Text("Lightning Model: %s", Settings.UseBlinnPhongShading ? "Blinn-Phong" : "Phong");
				ImGui::Checkbox("Blinn Phong Shading", &Settings.UseBlinnPhongShading);
				ImGui::Checkbox("Lightning", &Settings.UseLighting);
				ImGui::Checkbox("Diffuse Texture", &Settings.UseDiffuseTexture);
				ImGui::Checkbox("Specular Texture", &Settings.UseSpecularTexture);
				ImGui::Checkbox("Emission", &Settings.UseEmission);
				ImGui::Checkbox("Gamma Correction", &Settings.UseGamma);
				ImGui::SliderFloat("Gamma Value", &Settings.GammaValue, 1.0f / 2.2f, 2.2f);
				ImGui::Spacing();

				for (unsigned int i = 0; i < DirLights.size(); i++) {
					if (ImGui::TreeNode(std::string("Directional Light " + std::to_string(i)).c_str())) {
						DirLights[i]->GenerateDebugUI();
						ImGui::TreePop();
					}
					ImGui::Spacing();
				}

				for (unsigned int i = 0; i < PointLights.size(); i++) {
					if (ImGui::TreeNode(std::string("Point Light " + std::to_string(i)).c_str())) {
						PointLights[i]->GenerateDebugUI();
						ImGui::TreePop();
					}
					ImGui::Spacing();
				}

				for (unsigned int i = 0; i < SpotLights.size(); i++) {
					if (ImGui::TreeNode(std::string("Spot Light " + std::to_string(i)).c_str())) {
						SpotLights[i]->GenerateDebugUI();
						ImGui::TreePop();
					}
					ImGui::Spacing();
				}

				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Fog")) {
				fog->GenerateDebugUI();
				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Illustration")) {
				ImGui::Text("Current Screen: %d", Settings.CurrentDisplyMode);
				ImGui::Text("Showing Axes: %s", Settings.ShowOriginAnd3Axes ? "True" : "false");
				ImGui::Checkbox("Back Face Culling", &Settings.EnableBackFaceCulling); 
				ImGui::Checkbox("Enable Ball Culling", &enalbe_ball_culling);
				// ImGui::Text("Full Screen:  %s", isfullscreen ? "True" : "false");
				ImGui::Spacing();

				ImGui::EndTabItem();
			}
			
			ImGui::EndTabBar();
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::End();
	}

	void DrawOriginAnd3Axes(Nexus::Shader* shader) const {
		shader->SetBool("material.enableDiffuseTexture", false);
		shader->SetBool("material.enableSpecularTexture", false);
		shader->SetBool("material.enableEmission", true);
		shader->SetBool("material.enableEmissionTexture", false);
		
		// 繪製世界坐標系原點（0, 0, 0）
		model->Push();
		model->Save(glm::scale(model->Top(), glm::vec3(0.1f, 0.1f, 0.1f)));
		shader->SetVec4("material.ambient", glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
		shader->SetVec4("material.specular", glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
		shader->SetFloat("material.shininess", 64.0f);
		sphere->Draw(shader, model->Top());
		model->Pop();

		// 繪製三個軸
		model->Push();
		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(5.0f, 0.0f, 0.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(10.0f, 0.05f, 0.05f)));
		shader->SetVec4("material.ambient", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(1.0f, 0.0f, 0.0f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(1.0f, 0.0f, 0.0f, 1.0));
		shader->SetFloat("material.shininess", 64.0f);
		cube->Draw(shader, model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 5.0f, 0.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(0.05f, 10.0f, 0.05f)));
		shader->SetVec4("material.ambient", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(0.0f, 1.0f, 0.0f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(0.0f, 1.0f, 0.0f, 1.0));
		shader->SetFloat("material.shininess", 64.0f);
		cube->Draw(shader, model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 0.0f, 5.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(0.05f, 0.05f, 10.0f)));
		shader->SetVec4("material.ambient", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(0.0f, 0.0f, 1.0f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(0.0f, 0.0f, 1.0f, 1.0));
		shader->SetFloat("material.shininess", 64.0f);
		cube->Draw(shader, model->Top());
		model->Pop();
		model->Pop();
	}

	void SetViewMatrix(Nexus::DisplayMode monitor_type) {
		glm::vec3 camera_position = Settings.EnableGhostMode ? first_camera->GetPosition() : third_camera->GetPosition();
		switch (monitor_type) {
			case Nexus::DISPLAY_MODE_ORTHOGONAL_X:
				view = glm::lookAt(glm::vec3(10.0f, 10.0f, 0.0f), glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
				break;
			case Nexus::DISPLAY_MODE_ORTHOGONAL_Y:
				view = glm::lookAt(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0, 0.0, -1.0));
				break;
			case Nexus::DISPLAY_MODE_ORTHOGONAL_Z:
				view = glm::lookAt(glm::vec3(0.0f, 10.0f, 10.0f), glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
				break;
			case Nexus::DISPLAY_MODE_DEFAULT:
				view = Settings.EnableGhostMode ? first_camera->GetViewMatrix() : third_camera->GetViewMatrix();
				break;
		}
	}

	void SetProjectionMatrix(Nexus::DisplayMode monitor_type) {
		ProjectionSettings.Aspect = (float)Settings.Width / (float)Settings.Height;

		if (monitor_type == Nexus::DISPLAY_MODE_DEFAULT) {
			if (ProjectionSettings.IsPerspective) {
				projection = GetPerspectiveProjMatrix(glm::radians(Settings.EnableGhostMode ? first_camera->GetFOV() : third_camera->GetFOV()), ProjectionSettings.Aspect, ProjectionSettings.ClippingNear, ProjectionSettings.ClippingFar);
			} else {
				projection = GetOrthoProjMatrix(-ProjectionSettings.OrthogonalHeight * ProjectionSettings.Aspect, ProjectionSettings.OrthogonalHeight * ProjectionSettings.Aspect, -ProjectionSettings.OrthogonalHeight, ProjectionSettings.OrthogonalHeight, ProjectionSettings.ClippingNear, ProjectionSettings.ClippingFar);
			}
		} else {
			projection = GetOrthoProjMatrix(-11.0 * ProjectionSettings.Aspect, 11.0 * ProjectionSettings.Aspect, -11.0, 11.0, 0.01f, 500.0f);
		}
	}

	void SetViewport(Nexus::DisplayMode monitor_type) override {
		if (Settings.CurrentDisplyMode == Nexus::DISPLAY_MODE_3O1P) {
			switch (monitor_type) {
				case Nexus::DISPLAY_MODE_ORTHOGONAL_X:
					glViewport(0, Settings.Height / 2, Settings.Width / 2, Settings.Height / 2);
					break;
				case Nexus::DISPLAY_MODE_ORTHOGONAL_Y:
					glViewport(Settings.Width / 2, Settings.Height / 2, Settings.Width / 2, Settings.Height / 2);
					break;
				case Nexus::DISPLAY_MODE_ORTHOGONAL_Z:
					glViewport(0, 0, Settings.Width / 2, Settings.Height / 2);
					break;
				case Nexus::DISPLAY_MODE_DEFAULT:
					glViewport(Settings.Width / 2, 0, Settings.Width / 2, Settings.Height / 2);
					break;
			}
		} else {
			glViewport(0, 0, Settings.Width, Settings.Height);
		}
	}
	
	void OnWindowResize() override {
		ProjectionSettings.Aspect = (float)Settings.Width / (float)Settings.Height;

		// Reset viewport
		// SetViewport(Settings.CurrentDisplyMode);
	}
	
	void OnProcessInput(int key) override {
		if (Settings.EnableGhostMode) {
			if (key == GLFW_KEY_W) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_FORWARD, DeltaTime);
			}
			if (key == GLFW_KEY_S) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_BACKWARD, DeltaTime);
			}
			if (key == GLFW_KEY_A) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_LEFT, DeltaTime);
			}
			if (key == GLFW_KEY_D) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_RIGHT, DeltaTime);
			}
		}
	}
	
	void OnKeyPress(int key) override {
		if (key == GLFW_KEY_LEFT_SHIFT) {
			if (Settings.EnableGhostMode) {
				first_camera->SetMovementSpeed(50.0f);
			}
		}

		if (key == GLFW_KEY_X) {
			if (Settings.ShowOriginAnd3Axes) {
				Settings.ShowOriginAnd3Axes = false;
				Nexus::Logger::Message(Nexus::LOG_INFO, "World coordinate origin and 3 axes: [Hide].");
			} else {
				Settings.ShowOriginAnd3Axes = true;
				Nexus::Logger::Message(Nexus::LOG_INFO, "World coordinate origin and 3 axes: [Show].");
			}
		}

		if (key == GLFW_KEY_P) {
			if (ProjectionSettings.IsPerspective) {
				ProjectionSettings.IsPerspective = false;
				Nexus::Logger::Message(Nexus::LOG_INFO, "Projection Mode: Orthogonal");
			} else {
				ProjectionSettings.IsPerspective = true;
				Nexus::Logger::Message(Nexus::LOG_INFO, "Projection Mode: Perspective");
			}
		}

		if (key == GLFW_KEY_G) {
			if (Settings.EnableGhostMode) {
				Settings.EnableGhostMode = false;
				SpotLights[0]->SetEnable(false);
				SpotLights[1]->SetEnable(false);
				Nexus::Logger::Message(Nexus::LOG_INFO, "Camera Mode: Third Person");
			} else {
				Settings.EnableGhostMode = true;
				SpotLights[0]->SetEnable(false);
				SpotLights[1]->SetEnable(false);
				Nexus::Logger::Message(Nexus::LOG_INFO, "Camera Mode: First Person");
			}
		}

		// 手電筒開關
		if (key == GLFW_KEY_F) {
			if (Settings.EnableGhostMode) {
				if (SpotLights[1]->GetEnable()) {
					SpotLights[1]->SetEnable(false);
					Nexus::Logger::Message(Nexus::LOG_INFO, "Spot Light 1 is turn off.");
				} else {
					SpotLights[1]->SetEnable(true);
					Nexus::Logger::Message(Nexus::LOG_INFO, "Spot Light 1 is turn on.");
				}
			} else {
				if (SpotLights[0]->GetEnable()) {
					SpotLights[0]->SetEnable(false);
					Nexus::Logger::Message(Nexus::LOG_INFO, "Spot Light 0 is turn off.");
				}
				else {
					SpotLights[0]->SetEnable(true);
					Nexus::Logger::Message(Nexus::LOG_INFO, "Spot Light 0 is turn on.");
				}
			}
		}

		// 分鏡切換
		if (key == GLFW_KEY_1) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_ORTHOGONAL_X;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Orthogonal X.");
		}
		if (key == GLFW_KEY_2) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_ORTHOGONAL_Y;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Orthogonal Y.");
		}
		if (key == GLFW_KEY_3) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_ORTHOGONAL_Z;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Orthogonal Z.");
		}
		if (key == GLFW_KEY_4) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_DEFAULT;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Default Camera.");
		}
		if (key == GLFW_KEY_5) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_3O1P;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to All Screen.");
		}
	}
	
	void OnKeyRelease(int key) override {
		if (key == GLFW_KEY_LEFT_SHIFT) {
			if (Settings.EnableGhostMode) {
				first_camera->SetMovementSpeed(10.0f);
			}
		}
	}
	
	void OnMouseMove(int xoffset, int yoffset) override {
		if (!Settings.EnableCursor) {
			if (Settings.EnableGhostMode) {
				first_camera->ProcessMouseMovement(xoffset, yoffset);
			} else {
				third_camera->ProcessMouseMovement(xoffset, yoffset);
			}
		}
	}
	
	void OnMouseButtonPress(int button) override {
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			SetCursorDisable(true);
			Settings.EnableCursor = false;
		} 
	}
	
	void OnMouseButtonRelease(int button) override {
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			SetCursorDisable(false);
			Settings.EnableCursor = true;
		}
	}
	
	void OnMouseScroll(int yoffset) override {
		if (ProjectionSettings.IsPerspective) {
			if (Settings.EnableGhostMode) {
				first_camera->ProcessMouseScroll(yoffset);
			} else {
				third_camera->AdjustDistance(yoffset, 1.0f, 20.0f, 1.0f);
			}
		} else {
			AdjustOrthogonalProjectionWidth(yoffset);
		}
	}

	void AdjustOrthogonalProjectionWidth(float yoffset) {
		if (ProjectionSettings.OrthogonalHeight >= 1.0f && ProjectionSettings.OrthogonalHeight <= 100.0f) {
			ProjectionSettings.OrthogonalHeight -= (float)yoffset * 1.0f;
		}
		if (ProjectionSettings.OrthogonalHeight < 1.0f) {
			ProjectionSettings.OrthogonalHeight = 1.0f;
		}
		if (ProjectionSettings.OrthogonalHeight > 100.0f) {
			ProjectionSettings.OrthogonalHeight = 100.0f;
		}
	}

private:
	std::unique_ptr<Nexus::Shader> myShader = nullptr;
	std::unique_ptr<Nexus::Shader> normalShader = nullptr;
	
	std::unique_ptr<Nexus::FirstPersonCamera> first_camera = nullptr;
	std::unique_ptr<Nexus::ThirdPersonCamera> third_camera = nullptr;
	
	std::unique_ptr<Nexus::MatrixStack> model = nullptr;
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	std::unique_ptr<Nexus::Rectangle> floor = nullptr;
	std::unique_ptr<Nexus::Cube> cube = nullptr;
	std::unique_ptr<Nexus::Sphere> sphere = nullptr;
	std::unique_ptr<Nexus::ViewVolume> view_volume = nullptr;

	std::unique_ptr<Nexus::Texture2D> texture_checkerboard = nullptr;

	std::vector<Nexus::DirectionalLight*> DirLights;
	std::vector<Nexus::PointLight*> PointLights;
	std::vector<Nexus::SpotLight*> SpotLights;

	std::unique_ptr<Nexus::Fog> fog;

	std::vector<Ball> balls;
	std::vector<Obstacle> obstacles;
	bool enalbe_ball_culling = false;
};

int main() {
	NexusDemo app;
	return app.Run();
}