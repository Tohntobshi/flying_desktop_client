#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include "imgui/imgui.h"
#include "separateLoop.h"
#include "controlsSender.h"
#include "frameDecoder.h"


class Window: public SeparateLoop {
public:
  static Window * Init(ControlsSender * controlsSender, FrameDecoder * frameDecoder);
  static void Destroy();
private:
  ControlsSender * controlsSender = nullptr;
  FrameDecoder * frameDecoder = nullptr;
  void beforeLoop();
  void iterate();
  void afterLoop();
  static Window * instance;
  Window(ControlsSender * controlsSender, FrameDecoder * frameDecoder);
  // ~Window();
  SDL_Window * window = nullptr;
  // SDL_Renderer * renderer = nullptr;
  // SDL_Texture * texture = nullptr;
  SDL_GLContext glContext = NULL;
  ImGuiIO io;
  void prepareFrame();
  void drawGUI();
  void drawFrame();
  char someInput[128];
  GLuint vboId = 0;
  GLuint vertexShaderId = 0;
  GLuint fragmentShaderId = 0;
  GLuint shaderId = 0;
  GLint posAttrLoc = 0;
  GLint texPosAttrLoc = 0;
  GLuint texture = 0;
  uint8_t * lastFrame = nullptr;
};
