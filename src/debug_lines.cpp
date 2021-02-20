#include "debug_lines.h"

#define DEBUG_LINES_MAX 1024

struct DebugLine {
  vec3 start;
  vec3 end;
  vec3 rgb;
};

struct LineShader {
  GLuint program;
  GLint pos_loc;
  GLint color_loc;
  GLint view_proj_loc;
};

static DebugLine sDebugLines[DEBUG_LINES_MAX];
static int sDebugLinesCount;
static LineShader sLineShader;

int debug_lines_initialize() {
  sDebugLinesCount = 0;
  if(!(sLineShader.program = utility_create_program("shaders/line.vert", "shaders/line.frag"))) {
    return 1;
  }
  if (utility_link_program(sLineShader.program)) {
    return 1;
  }
  GL_WRAP(sLineShader.pos_loc = glGetAttribLocation(sLineShader.program, "position"));
  GL_WRAP(sLineShader.color_loc = glGetAttribLocation(sLineShader.program, "color"));
  GL_WRAP(sLineShader.view_proj_loc = glGetUniformLocation(sLineShader.program, "ViewProj"));
  return 0;
}

static void debug_lines_submit_min_max(const vec3 min, const vec3 max, const vec3 rgb) {
  debug_lines_submit(max[0], max[1], max[2], min[0], max[1], max[2], rgb);
  debug_lines_submit(max[0], max[1], max[2], max[0], min[1], max[2], rgb);
  debug_lines_submit(max[0], max[1], max[2], max[0], max[1], min[2], rgb);
  debug_lines_submit(min[0], min[1], min[2], max[0], min[1], min[2], rgb);
  debug_lines_submit(min[0], min[1], min[2], min[0], max[1], min[2], rgb);
  debug_lines_submit(min[0], min[1], min[2], min[0], min[1], max[2], rgb);
  debug_lines_submit(min[0], min[1], max[2], min[0], max[1], max[2], rgb);
  debug_lines_submit(min[0], min[1], max[2], max[0], min[1], max[2], rgb);
  debug_lines_submit(max[0], min[1], min[2], max[0], max[1], min[2], rgb);
  debug_lines_submit(max[0], min[1], min[2], max[0], min[1], max[2], rgb);
  debug_lines_submit(min[0], max[1], min[2], max[0], max[1], min[2], rgb);
  debug_lines_submit(min[0], max[1], min[2], min[0], max[1], max[2], rgb);
}

void debug_lines_submit_cube(const vec3 center, float sides, const vec3 rgb) {
  vec3 min, max, half_extents;
  vec3_swizzle(half_extents, sides/2.0f);
  vec3_sub(min, center, half_extents);
  vec3_add(max, center, half_extents);
  debug_lines_submit_min_max(min, max, rgb);
}

void debug_lines_submit_aabb(const Bounds* b, const vec3 rgb) {
  vec3 min, max;
  bounds_to_min_max(b, min, max);
  debug_lines_submit_min_max(min, max, rgb);
}

void debug_lines_submit_obb(const OBB* obb, const vec3 rgb) {
  mat4x4 m;
  mat4x4_identity(m);
  vec3_dup(&m[3][0], obb->center);
  for (int i = 0; i < 3; i++) {
    vec3_scale(&m[i][0], obb->axes[i], obb->extents[i]);
  }
  vec4 box[8] = {
    {  1.0f,  1.0f,  1.0f, 1.0f },
    {  1.0f, -1.0f,  1.0f, 1.0f },
    { -1.0f, -1.0f,  1.0f, 1.0f },
    { -1.0f,  1.0f,  1.0f, 1.0f },
    {  1.0f,  1.0f, -1.0f, 1.0f },
    {  1.0f, -1.0f, -1.0f, 1.0f },
    { -1.0f, -1.0f, -1.0f, 1.0f },
    { -1.0f,  1.0f, -1.0f, 1.0f },
  };
  for (int i = 0; i < 8; i++) {
    vec4 tmp;
    vec4_dup(tmp, box[i]);
    mat4x4_mul_vec4(box[i], m, tmp);
  }
  for (int i = 0; i < 4; i++) {
    debug_lines_submit(box[i], box[(i+1) % 4], rgb); // plane z
    debug_lines_submit(box[i+4], box[4 + ((i+1) % 4)], rgb); // plaze -z
    debug_lines_submit(box[i], box[i+4], rgb); // cross axis
  }
}

void debug_lines_submit(const vec3 start, const vec3 end, const vec3 rgb) {
  debug_lines_submit(
    start[0], start[1], start[2],
    end[0], end[1], end[2],
    rgb
  );
}

void debug_lines_submit(float start_x, float start_y, float start_z
  , float end_x, float end_y, float end_z
  , const vec3 rgb) {
    if (!(sDebugLinesCount >= DEBUG_LINES_MAX)) {
      DebugLine* line = &sDebugLines[sDebugLinesCount++];
      vec3_set(line->start, start_x, start_y, start_z);
      vec3_set(line->end, end_x, end_y, end_z);
      vec3_dup(line->rgb, rgb);
    }
}

void debug_lines_clear() {
  sDebugLinesCount = 0;
}

void debug_lines_render(const Scene *s) {
  GL_WRAP(glUseProgram(sLineShader.program));
  GL_WRAP(glDisable(GL_BLEND));
  GL_WRAP(glDisable(GL_DEPTH_TEST));
  GL_WRAP(glDepthMask(GL_FALSE));

  GL_WRAP(glUniformMatrix4fv(sLineShader.view_proj_loc, 1, GL_FALSE, (const GLfloat*)s->camera.viewProj));

  GL_WRAP(
    glBegin(GL_LINES);
    for (int i = 0; i < sDebugLinesCount; i++) {
      glVertexAttrib3fv(sLineShader.color_loc, sDebugLines[i].rgb);
      glVertexAttrib3fv(sLineShader.pos_loc, sDebugLines[i].start);
      glVertexAttrib3fv(sLineShader.color_loc, sDebugLines[i].rgb);
      glVertexAttrib3fv(sLineShader.pos_loc, sDebugLines[i].end);
    }
    glEnd()
  );

  GL_WRAP(glDepthMask(GL_TRUE));
}
