#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include "imgui/imgui.h"
#include "separateLoop.h"
#include "controlsSender.h"
#include "frameDecoder.h"
#include "debugReceiver.h"

struct ControlledValues {
  int baseVal = 0;

  float pitchPropCoef = 0.f;
  float pitchDerCoef = 0.f;
  float pitchIntCoef = 0.f;

  float rollPropCoef = 0.f;
  float rollDerCoef = 0.f;
  float rollIntCoef = 0.f;

  float yawSpPropCoef = 0.0f;
  float yawSpDerCoef = 0.f;
  float yawSpIntCoef = 0.f;

  float pitchBias = 0.f;
  float rollBias = 0.f;
  float yawSpeedBias = 0.f;

  float accTrust = 0.1f;
  float inclineChangeRateFilteringCoef = 0.f;
  float yawSpeedFilteringCoef = 0.f;
  float yawSpeedChangeRateFilteringCoef = 0.f;

  bool onlyPositiveAdjustMode = true;
};

class Window: public SeparateLoop {
public:
  static Window * Init(ControlsSender * controlsSender, FrameDecoder * frameDecoder, DebugReceiver * debugReceiver);
  static void Destroy();
private:
  ControlsSender * controlsSender = nullptr;
  FrameDecoder * frameDecoder = nullptr;
  DebugReceiver * debugReceiver = nullptr;
  void beforeLoop();
  void iterate();
  void afterLoop();
  static Window * instance;
  Window(ControlsSender * controlsSender, FrameDecoder * frameDecoder, DebugReceiver * debugReceiver);
  // ~Window();
  SDL_Window * window = nullptr;
  // SDL_Renderer * renderer = nullptr;
  // SDL_Texture * texture = nullptr;
  SDL_GLContext glContext = NULL;
  ImGuiIO io;
  void prepareFrame();
  void drawGUI();
  void updatePlots();
  void drawFrame();
  void handleEvent(SDL_Event& e);
  GLuint vboId = 0;
  GLuint vertexShaderId = 0;
  GLuint fragmentShaderId = 0;
  GLuint shaderId = 0;
  GLint posAttrLoc = 0;
  GLint texPosAttrLoc = 0;
  GLuint texture = 0;
  uint8_t * lastFrame = nullptr;


  int plotLength = 90;
  float pitchErPlot[90] = { 0 };
  float rollErPlot[90] = { 0 };
  float pitchErChRPlot[90] = { 0 };
  float rollErChRPlot[90] = { 0 };
  float yawSpErPlot[90] = { 0 };
  float yawSpErChRPlot[90] = { 0 };

  ControlledValues contVals;
  ControlledValues prevContVals;
  void compareControlledValsAndSendCommands();
  void resetContValues();
  void saveProfile();
  void loadProfile();
  char filename[128];
};
