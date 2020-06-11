#include "window.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#define GL_GLEXT_PROTOTYPES 1


Window * Window::instance = nullptr;

Window * Window::Init(ControlsSender * cs, FrameDecoder * fd, DebugReceiver * dr)
{
  if (instance != nullptr)
  {
    return instance;
  }
  instance = new Window(cs, fd, dr);
  return instance;
}

void Window::Destroy()
{
  delete instance;
  instance = nullptr;
}

Window::Window(ControlsSender * cs, FrameDecoder * fd, DebugReceiver * dr): controlsSender(cs), frameDecoder(fd), debugReceiver(dr)
{
}

void Window::beforeLoop()
{
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window = SDL_CreateWindow("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 1024, SDL_WINDOW_OPENGL);
  // renderer = SDL_CreateRenderer(window, -1, 0);
  // texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, 640, 480);

  glContext = SDL_GL_CreateContext(window);

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  // glEnable(GL_CULL_FACE);
  // glCullFace(GL_BACK);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, glContext);
  ImGui_ImplOpenGL3_Init("#version 100");
  io = ImGui::GetIO();

  prepareFrame();
}

void Window::afterLoop()
{
  delete [] lastFrame;

  glDeleteBuffers(1, &vboId);
  glDeleteShader(vertexShaderId);
  glDeleteShader(fragmentShaderId);
  glDeleteProgram(shaderId);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DeleteContext(glContext);

  // SDL_DestroyRenderer(renderer);
  // SDL_DestroyTexture(texture);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void Window::iterate()
{
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  compareControlledValsAndSendCommands();

  SDL_Event event;
  while (SDL_PollEvent(&event)) handleEvent(event);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();

  updatePlots();
  drawFrame();
  drawGUI();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(window);
}

void Window::prepareFrame()
{
  // create model coord
  float coords[24] = {
    -1.f, -1.f, 0.f, 0.f,
    -1.f, 1.f, 0.f, 1.f,
    1.f, 1.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f,
    1.f, -1.f, 1.f, 0.f,
    -1.f, -1.f, 0.f, 0.f,
  };
  glGenBuffers(1, &vboId);
  glBindBuffer(GL_ARRAY_BUFFER, vboId);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, coords, GL_STATIC_DRAW);

  // create shader
  std::string vertex = R"GLSL(
    attribute vec2 positionAttr;
    attribute vec2 texPositionAttr;
    varying vec2 texPos;
    void main()
    {
      texPos = texPositionAttr;
      gl_Position = vec4(positionAttr, 0.0, 1.0);
    }
  )GLSL";
  std::string fragment = R"GLSL(
    precision mediump float;
    varying vec2 texPos;
    uniform sampler2D tex;
    void main()
    {
      vec4 color = texture2D(tex, texPos);
      gl_FragColor = color;
    }
  )GLSL";
  const char* _vertexSource = vertex.c_str();
  const char* _fragmentSource = fragment.c_str();
  vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShaderId, 1, &_vertexSource, NULL);
  glCompileShader(vertexShaderId);

  fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShaderId, 1, &_fragmentSource, NULL);
  glCompileShader(fragmentShaderId);

  shaderId = glCreateProgram();
  glAttachShader(shaderId, vertexShaderId);
  glAttachShader(shaderId, fragmentShaderId);
  glLinkProgram(shaderId);

  posAttrLoc = glGetAttribLocation(shaderId, "positionAttr");
  texPosAttrLoc = glGetAttribLocation(shaderId, "texPositionAttr");
  glEnableVertexAttribArray(posAttrLoc);
  glEnableVertexAttribArray(texPosAttrLoc);
}

void Window::drawFrame()
{
  //   SDL_UpdateTexture(texture, nullptr, lastFrame, 2 * 640);
  //   SDL_RenderCopy(renderer, texture, NULL, NULL);
  //   SDL_RenderPresent(renderer);
  glUseProgram(shaderId);
  glBindBuffer(GL_ARRAY_BUFFER, vboId);
  glVertexAttribPointer(posAttrLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
  glVertexAttribPointer(texPosAttrLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

  if (frameDecoder != nullptr) {
    uint8_t * lastFr = frameDecoder->getLastFrame();
    if (lastFr != nullptr)
    {
      delete [] lastFrame;
      lastFrame = lastFr;
    }
  }
  if (lastFrame != nullptr)
  {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, lastFrame);
    glBindTexture(GL_TEXTURE_2D, texture);
  }

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glDeleteTextures(1, &texture);
}

void Window::drawGUI()
{
  // ImGui::ShowDemoWindow();
  ImGui::Begin("Pitch adjust");
    ImGui::Text("pitch error: %f", pitchErPlot[plotLength - 1]);
    ImGui::PlotLines("pitch error", pitchErPlot, plotLength, 0, nullptr, -90.0f, 90.0f, ImVec2(0,80));
    ImGui::Text("pitch error change rate: %f", pitchErChRPlot[plotLength - 1]);
    ImGui::PlotLines("pitch error change rate", pitchErChRPlot, plotLength, 0, nullptr, -180.0f, 180.0f, ImVec2(0,80));
    ImGui::DragFloat("incline prop coef", &contVals.pitchPropCoef, 0.005f);
    ImGui::DragFloat("incline der coef", &contVals.pitchDerCoef, 0.005f);
    ImGui::DragFloat("incline int coef", &contVals.pitchIntCoef, 0.005f);
    ImGui::DragFloat("pitch bias", &contVals.pitchBias, 0.005f);
  ImGui::End();

  ImGui::Begin("Roll adjust");
    ImGui::Text("roll error: %f", rollErPlot[plotLength - 1]);
    ImGui::PlotLines("roll error", rollErPlot, plotLength, 0, nullptr, -90.0f, 90.0f, ImVec2(0,80));
    ImGui::Text("pitch error: %f", rollErChRPlot[plotLength - 1]);
    ImGui::PlotLines("roll error change rate", rollErChRPlot, plotLength, 0, nullptr, -180.0f, 180.0f, ImVec2(0,80));
    ImGui::DragFloat("incline prop coef", &contVals.rollPropCoef, 0.005f);
    ImGui::DragFloat("incline der coef", &contVals.rollDerCoef, 0.005f);
    ImGui::DragFloat("incline int coef", &contVals.rollIntCoef, 0.005f);
    ImGui::DragFloat("roll bias", &contVals.rollBias, 0.005f);
  ImGui::End();

  ImGui::Begin("Yaw adjust");
    ImGui::Text("yaw speed error: %f", yawSpErPlot[plotLength - 1]);
    ImGui::PlotLines("yaw speed error", yawSpErPlot, plotLength, 0, nullptr, -180.0f, 180.0f, ImVec2(0,80));
    ImGui::Text("yaw speed error change rate: %f", yawSpErChRPlot[plotLength - 1]);
    ImGui::PlotLines("yaw speed error change rate", yawSpErChRPlot, plotLength, 0, nullptr, -360.0f, 360.0f, ImVec2(0,80));
    ImGui::DragFloat("yaw sp prop coef", &contVals.yawSpPropCoef, 0.005f);
    ImGui::DragFloat("yaw sp der coef", &contVals.yawSpDerCoef, 0.005f);
    ImGui::DragFloat("yaw sp int coef", &contVals.yawSpIntCoef, 0.005f);
    ImGui::DragFloat("yaw speed bias", &contVals.yawSpeedBias, 0.005f);
  ImGui::End();


  ImGui::Begin("Base val");
    ImGui::SliderInt("base val", &contVals.baseVal, 1000, 2000, "base val %d");
    ImGui::Checkbox("only positive adjust", &contVals.onlyPositiveAdjustMode);
    if (ImGui::Button("Reset"))
    {
      resetContValues();
    }
    if (ImGui::Button("Arm") && controlsSender != nullptr)
    {
      controlsSender->sendCommand(ARM);
    }
    if (ImGui::Button("Calibrate") && controlsSender != nullptr)
    {
      controlsSender->sendCommand(CALIBRATE);
    }
  ImGui::End();

  ImGui::Begin("reducing noise");
    ImGui::SliderFloat("acc trust", &contVals.accTrust, 0.f, 1.f);
    ImGui::SliderFloat("incline err ch rate filter", &contVals.inclineChangeRateFilteringCoef, 0.f, 1.f);
    ImGui::SliderFloat("yaw sp filter", &contVals.yawSpeedFilteringCoef, 0.f, 1.f);
    ImGui::SliderFloat("yaw sp err ch rate filter", &contVals.yawSpeedChangeRateFilteringCoef, 0.f, 1.f);
  ImGui::End();
}

void Window::updatePlots()
{
  while (true && debugReceiver != nullptr)
  {
    DebugInfo info = debugReceiver->getInfo();
    if (info.noMoreInfo) break;
    for (int i = 0; i < plotLength; i++)
    {
      if (i == 0) continue;
      pitchErPlot[i - 1] = pitchErPlot[i];
      rollErPlot[i - 1] = rollErPlot[i];
      pitchErChRPlot[i - 1] = pitchErChRPlot[i];
      rollErChRPlot[i - 1] = rollErChRPlot[i];
      yawSpErPlot[i - 1] = yawSpErPlot[i];
      yawSpErChRPlot[i - 1] = yawSpErChRPlot[i];
    }
    pitchErPlot[plotLength - 1] = info.pitchError;
    rollErPlot[plotLength - 1] = info.rollError;
    pitchErChRPlot[plotLength - 1] = info.pitchErrorChangeRate;
    rollErChRPlot[plotLength - 1] = info.rollErrorChangeRate;
    yawSpErPlot[plotLength - 1] = info.yawSpeedError;
    yawSpErChRPlot[plotLength - 1] = info.yawSpeedErrorChangeRate;
  }
}

void Window::compareControlledValsAndSendCommands()
{
  if (controlsSender == nullptr) return;


  if (prevContVals.pitchPropCoef != contVals.pitchPropCoef)
  {
    controlsSender->sendValue(SET_PITCH_PROP_COEF, contVals.pitchPropCoef);
    prevContVals.pitchPropCoef = contVals.pitchPropCoef;
  }
  if (prevContVals.pitchDerCoef != contVals.pitchDerCoef)
  {
    controlsSender->sendValue(SET_PITCH_DER_COEF, contVals.pitchDerCoef);
    prevContVals.pitchDerCoef = contVals.pitchDerCoef;
  }
  if (prevContVals.pitchIntCoef != contVals.pitchIntCoef)
  {
    controlsSender->sendValue(SET_PITCH_INT_COEF, contVals.pitchIntCoef);
    prevContVals.pitchIntCoef = contVals.pitchIntCoef;
  }


  if (prevContVals.rollPropCoef != contVals.rollPropCoef)
  {
    controlsSender->sendValue(SET_ROLL_PROP_COEF, contVals.rollPropCoef);
    prevContVals.rollPropCoef = contVals.rollPropCoef;
  }
  if (prevContVals.rollDerCoef != contVals.rollDerCoef)
  {
    controlsSender->sendValue(SET_ROLL_DER_COEF, contVals.rollDerCoef);
    prevContVals.rollDerCoef = contVals.rollDerCoef;
  }
  if (prevContVals.rollIntCoef != contVals.rollIntCoef)
  {
    controlsSender->sendValue(SET_ROLL_INT_COEF, contVals.rollIntCoef);
    prevContVals.rollIntCoef = contVals.rollIntCoef;
  }


  if (prevContVals.yawSpPropCoef != contVals.yawSpPropCoef)
  {
    controlsSender->sendValue(SET_YAW_SP_PROP_COEF, contVals.yawSpPropCoef);
    prevContVals.yawSpPropCoef = contVals.yawSpPropCoef;
  }
  if (prevContVals.yawSpDerCoef != contVals.yawSpDerCoef)
  {
    controlsSender->sendValue(SET_YAW_SP_DER_COEF, contVals.yawSpDerCoef);
    prevContVals.yawSpDerCoef = contVals.yawSpDerCoef;
  }
  if (prevContVals.yawSpIntCoef != contVals.yawSpIntCoef)
  {
    controlsSender->sendValue(SET_YAW_SP_INT_COEF, contVals.yawSpIntCoef);
    prevContVals.yawSpIntCoef = contVals.yawSpIntCoef;
  }


  if (prevContVals.pitchBias != contVals.pitchBias)
  {
    controlsSender->sendValue(SET_PITCH_BIAS, contVals.pitchBias);
    prevContVals.pitchBias = contVals.pitchBias;
  }
  if (prevContVals.rollBias != contVals.rollBias)
  {
    controlsSender->sendValue(SET_ROLL_BIAS, contVals.rollBias);
    prevContVals.rollBias = contVals.rollBias;
  }
  if (prevContVals.yawSpeedBias != contVals.yawSpeedBias)
  {
    controlsSender->sendValue(SET_YAW_SPEED_BIAS, contVals.yawSpeedBias);
    prevContVals.yawSpeedBias = contVals.yawSpeedBias;
  }


  if (prevContVals.baseVal != contVals.baseVal)
  {
    controlsSender->sendValue(SET_BASE_VAL, contVals.baseVal);
    prevContVals.baseVal = contVals.baseVal;
  }


  if (prevContVals.accTrust != contVals.accTrust)
  {
    controlsSender->sendValue(SET_ACC_TRUST, contVals.accTrust);
    prevContVals.accTrust = contVals.accTrust;
  }
  if (prevContVals.inclineChangeRateFilteringCoef != contVals.inclineChangeRateFilteringCoef)
  {
    controlsSender->sendValue(SET_INC_CH_RATE_FILTER_COEF, contVals.inclineChangeRateFilteringCoef);
    prevContVals.inclineChangeRateFilteringCoef = contVals.inclineChangeRateFilteringCoef;
  }
  if (prevContVals.yawSpeedFilteringCoef != contVals.yawSpeedFilteringCoef)
  {
    controlsSender->sendValue(SET_YAW_SP_FILTER_COEF, contVals.yawSpeedFilteringCoef);
    prevContVals.yawSpeedFilteringCoef = contVals.yawSpeedFilteringCoef;
  }
  if (prevContVals.yawSpeedChangeRateFilteringCoef != contVals.yawSpeedChangeRateFilteringCoef)
  {
    controlsSender->sendValue(SET_YAW_SP_CH_RATE_FILTER_COEF, contVals.yawSpeedChangeRateFilteringCoef);
    prevContVals.yawSpeedChangeRateFilteringCoef = contVals.yawSpeedChangeRateFilteringCoef;
  }

  if (prevContVals.onlyPositiveAdjustMode != contVals.onlyPositiveAdjustMode)
  {
    controlsSender->sendValue(SET_ONLY_POSITIVE_ADJUST_MODE, (int)contVals.onlyPositiveAdjustMode);
    prevContVals.onlyPositiveAdjustMode = contVals.onlyPositiveAdjustMode;
  }
}

void Window::resetContValues()
{
  ControlledValues cont;
  contVals = cont;
}

void Window::handleEvent(SDL_Event & e)
{
  ImGui_ImplSDL2_ProcessEvent(&e);
  if (e.type == SDL_QUIT)
  {
    stop();
    if (controlsSender != nullptr) controlsSender->stop();
    if (frameDecoder != nullptr) frameDecoder->stop();
    if (debugReceiver != nullptr) debugReceiver->stop();
    return;
  }
  if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_SPACE && !io.WantCaptureKeyboard) contVals.baseVal = 1000;
  if (controlsSender == nullptr) return;
  controlsSender->handleEvent(e);
}