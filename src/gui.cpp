#include "gui.h"
#include "assets.h"

#include "imgui/imgui.h"
#include "imgui/imgui_custom_theme.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/ImGuizmo.h"

static int show_manipulator = 0;

void gui_initialize(SDL_Window* window) {
  // init Imgui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForOpenGL(window, SDL_GL_GetCurrentContext());
  ImGui_ImplOpenGL3_Init("#version 130");
  StyleImguiCustom();
}

void gui_destroy() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void gui_render_loading_screen(SDL_Window* window, const char* stage, const char* asset, int index, int total) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();

  ImGui::SetNextWindowPosCenter();
  ImGui::SetNextWindowSize(ImVec2(400, 100));
  ImGui::Begin("PBR Renderer - Loading", 0,
    ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
  );
    if (total > 0) {
      ImGui::Text("%s...    %s (%i / %i)", stage, asset, index, total);
    } else {
      ImGui::Text("%s...", stage);
    }
    ImGui::ProgressBar((float)index / (float)total, ImVec2(-1, 8), " ");
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void gui_begin_frame(SDL_Window* window) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();
  ImGuizmo::BeginFrame();
}

void gui_end_frame() {
  // ImGui::ShowDemoWindow();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int gui_process_event(SDL_Event* event) {
  if (ImGui::GetIO().WantCaptureMouse) {
    ImGui_ImplSDL2_ProcessEvent(event);
    return 1; // ImGui handled this event
  }
  return 0;
}

void gui_rotation_gizmo(quat out_euler, const vec3 pos, const mat4x4 view, const mat4x4 proj) {
  mat4x4 manip_mat;
  mat4x4_identity(manip_mat);
  mat4x4_translate(manip_mat, pos[0], pos[1], pos[2]);
  mat4x4_rotate_Z(manip_mat, manip_mat, DEG_TO_RAD(out_euler[2]));
  mat4x4_rotate_Y(manip_mat, manip_mat, DEG_TO_RAD(out_euler[1]));
  mat4x4_rotate_X(manip_mat, manip_mat, DEG_TO_RAD(out_euler[0]));
  ImGuizmo::Enable(true);
  ImGuizmo::SetRect((float)VIEWPORT_X_OFFSET, 0.0f, (float)VIEWPORT_WIDTH, (float)VIEWPORT_HEIGHT);
  ImGuizmo::Manipulate(
    &view[0][0],
    &proj[0][0],
    ImGuizmo::ROTATE,
    ImGuizmo::LOCAL,
    &manip_mat[0][0]
  );
  mat4x4_to_euler(out_euler, manip_mat);
  for (int i = 0; i < 3; i++) {
    out_euler[i] = RAD_TO_DEG(out_euler[i]);
  }
}

void gui_translation_gizmo(vec3 out_pos, const mat4x4 view, const mat4x4 proj) {
  mat4x4 manip_mat;
  mat4x4_identity(manip_mat);
  vec3_dup(manip_mat[3], out_pos);
  ImGuizmo::Enable(true);
  ImGuizmo::SetRect((float)VIEWPORT_X_OFFSET, 0.0f, (float)VIEWPORT_WIDTH, (float)VIEWPORT_HEIGHT);
  ImGuizmo::Manipulate(
    &view[0][0],
    &proj[0][0],
    ImGuizmo::TRANSLATE,
    ImGuizmo::LOCAL,
    &manip_mat[0][0]
  );
  vec3_dup(out_pos, manip_mat[3]);
}

void gui_render(SDL_Window* window, Renderer* renderer, Scene* scene, float dt, EditorState* state) {
  gui_begin_frame(window);
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(SIDEBAR_WIDTH, WINDOW_HEIGHT));
  ImGui::Begin("Renderer Controls", 0,
    ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
  );
  ImGui::Combo("Render Mode", (int*)&renderer->deferred.render_mode, render_mode_strings, render_mode_strings_count);
  ImGui::Combo("Tonemapping Operator", (int*)&renderer->deferred.tonemapping_op, tonemapping_op_strings, tonemapping_op_strings_count);
  ImGui::Checkbox("Show Debug Lines", (bool*)&renderer->render_debug_lines);
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Auto Rotate", (bool*)&scene->camera.auto_rotate);
    ImGui::SliderFloat("Cam Zoom", (float*)&scene->camera.boom_len, 0.0f, 150.0f);
    ImGui::SliderFloat("FOVy", (float*)&scene->camera.fovy, 0.0f, 180.0f);
  }
  if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Combo("Skybox Render Mode", (int*)&renderer->deferred.skybox_mode, skybox_mode_strings, skybox_mode_strings_count);
    if (renderer->deferred.skybox_mode == SKYBOX_MODE_PREFILTER_MAP) {
      ImGui::SliderFloat("LoD", (float*)&renderer->deferred.prefilter_lod, 0.0f, 10.0f);
    }
    const SkyboxDesc* sd = scene->skybox->desc;
    if (ImGui::BeginCombo("Skybox", sd->name, 0)) {
      for (int i = 0; i < gSkyboxesCount; i++) {
        if (ImGui::Selectable(gSkyboxes[i].name, (&gSkyboxes[i] == sd))) {
          scene->skybox = &gSkyboxes[i].skybox;
        }
      }
      ImGui::EndCombo();
    }
    ImGui::ColorEdit3("Ambient Color", scene->ambient_color);
    ImGui::SliderFloat("Ambient Intensity", &scene->ambient_intensity, 0, 6.0f);
  }
  if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::PushID("light");
    bool show_light_manipulator = (show_manipulator == 1);
    if (ImGui::Checkbox("Show Manipulator##light", &show_light_manipulator)) {
      show_manipulator = (show_light_manipulator) ? 1 : 0;
    }
    light_gui(scene->light);
    ImGui::PopID();
  }
  if (ImGui::CollapsingHeader("Shadow Map", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Show Debug View", (bool*)&renderer->debug_shadow_map);
  }
  if (ImGui::Button("Save Screenshot")) {
    if (!utility_save_screenshot("./test.png", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT)) {
      printf("Wrote screenshot to './test.png'\n");
    } else {
      printf("Error saving screenshot\n");
    }
  }
  if (ImGui::Button("Save Shadow Map Screenshot")) {
    if (!utility_save_depth_screenshot("./depth-test.png", renderer->shadow_map.fbo, 0, 0, renderer->shadow_map.width, renderer->shadow_map.height)) {
      printf("Wrote screenshot to './depth-test.png'\n");
    } else {
      printf("Error saving screenshot\n");
    }
  }
  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(SIDEBAR_WIDTH + VIEWPORT_WIDTH, 0));
  ImGui::SetNextWindowSize(ImVec2(SIDEBAR_WIDTH, WINDOW_HEIGHT));
  ImGui::Begin("Model Controls", 0,
    ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
  );
  if (ImGui::BeginTabBar("ModelTabBar", ImGuiTabBarFlags_None))
  {
    if (ImGui::BeginTabItem("Model"))
    {
      if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("model");
        const MeshDesc* md = scene->models[0]->mesh->desc;
        if (ImGui::BeginCombo("Geometry", md->name, 0)) {
          for (int i = 0; i < gMeshesCount; i++) {
            if (ImGui::Selectable(gMeshes[i].name, (&gMeshes[i] == md))) {
              scene->models[0]->mesh = &gMeshes[i].mesh;
            }
          }
          ImGui::EndCombo();
        }
        ImGui::SliderFloat("Rotation (Deg)", &scene->models[0]->rot[1], 0.0f, 360.0f, "%.0f");
        ImGui::SliderFloat("Scale", &scene->models[0]->scale, .01f, 25.0f );
        bool show_model_trans_manip = (show_manipulator == 2);
        if (ImGui::Checkbox("Translation Manipulator", &show_model_trans_manip)) {
          show_manipulator = (show_model_trans_manip) ? 2 : 0;
        }
        bool show_model_rot_manip = (show_manipulator == 3);
        if (ImGui::Checkbox("Rotation Show Manipulator", &show_model_rot_manip)) {
          show_manipulator = (show_model_rot_manip) ? 3 : 0;
        }
        if (ImGui::Button("Reset")) {
          vec3_zero(scene->models[0]->position);
          vec3_zero(scene->models[0]->rot);
        }
        ImGui::PopID();
      }
      if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        material_gui(&scene->models[0]->material);
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Emitter")) {
      particle_emitter_gui(scene->emitters[0], gParticleTextures, gParticleTexturesCount);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(SIDEBAR_WIDTH + 10, VIEWPORT_HEIGHT - 10), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
  ImGui::SetNextWindowBgAlpha(0.35f);
  if (ImGui::Begin("FPS", 0,
    ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoDecoration
    | ImGuiWindowFlags_AlwaysAutoResize
    | ImGuiWindowFlags_NoSavedSettings
    | ImGuiWindowFlags_NoFocusOnAppearing
    | ImGuiWindowFlags_NoNav
  )) {
    int fps = 1.0f / dt;
    ImGui::Text("FPS: %i", (int)fps);
  }
  ImGui::End();


  ImGui::SetNextWindowPos(ImVec2(SIDEBAR_WIDTH  + VIEWPORT_WIDTH - 10, VIEWPORT_HEIGHT - 10), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
  // ImGui::SetNextWindowSize(ImVec2(100, 0));
  ImGui::SetNextWindowBgAlpha(0.65f);
  if (ImGui::Begin("Time Controls", 0,
    ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoDecoration
    | ImGuiWindowFlags_AlwaysAutoResize
    | ImGuiWindowFlags_NoSavedSettings
    | ImGuiWindowFlags_NoFocusOnAppearing
    | ImGuiWindowFlags_NoNav
  )) {
    if (!state->paused) {
      if (ImGui::Button("Pause", ImVec2(60,0))) {
        state->paused = 1;
      }
    } else {
      if (ImGui::Button("Play", ImVec2(60,0))) {
        state->paused = 0;
      }
    }
    ImGui::SameLine();
    state->step_frame = ImGui::Button("Step", ImVec2(60,0));
    ImGui::Text("Physics Sim: %s", state->paused ? "Paused" : "Running");
    ImGui::Checkbox("Pause on collision", (bool*)&state->pause_on_collision);
    ImGui::SliderFloat("Time Scale", (float*)&state->time_scale, 0.0f, 1.0f);

    ImGui::SliderFloat("Restitution", (float*)&state->restitution, 0.0f, 1.0f);
    ImGui::SliderFloat("Friction", (float*)&state->friction, 0.0f, 1.0f);
    ImGui::SliderFloat("Mass", (float*)&state->mass, 0.0f, 100.0f);
    ImGui::SliderFloat("Liner Damping", (float*)&state->linear_damping, 0.0f, 1.0f);
    ImGui::SliderFloat("Angular Damping", (float*)&state->angular_damping, 0.0f, 1.0f);
  }
  ImGui::End();

  // render translation gizmo
  switch (show_manipulator) {
    case 1: {
      if (scene->light->type == LIGHT_TYPE_POINT) {
        gui_translation_gizmo(scene->light->position, scene->camera.view, scene->camera.proj);
      } else {
        gui_rotation_gizmo(scene->light->rot, scene->light->position, scene->camera.view, scene->camera.proj);
      }
      break;
    }
    case 2: gui_translation_gizmo(scene->models[0]->position, scene->camera.view, scene->camera.proj); break;
    case 3: gui_rotation_gizmo(scene->models[0]->rot, scene->models[0]->position, scene->camera.view, scene->camera.proj); break;
  }
  gui_end_frame();
}
