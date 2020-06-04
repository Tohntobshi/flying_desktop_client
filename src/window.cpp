#include "window.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#define GL_GLEXT_PROTOTYPES 1


Window * Window::instance = nullptr;

Window * Window::Init(ControlsSender * cs, FrameDecoder * fd)
{
  if (instance != nullptr)
  {
    return instance;
  }
  instance = new Window(cs, fd);
  return instance;
}

void Window::Destroy()
{
  delete instance;
  instance = nullptr;
}

Window::Window(ControlsSender * cs, FrameDecoder * fd): controlsSender(cs), frameDecoder(fd)
{
}

void Window::beforeLoop()
{
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window = SDL_CreateWindow("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_OPENGL);
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
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
    {
      stop();
      if (controlsSender != nullptr) controlsSender->stop();
      if (frameDecoder != nullptr) frameDecoder->stop();
    }
    else
    {
      if (controlsSender != nullptr) controlsSender->handleEvent(event);
    }
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();

  drawFrame();
  // drawGUI();
  ImGui::ShowDemoWindow();

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
  ImGui::Begin("Controls");
  if (ImGui::CollapsingHeader("Title"))
  {
    ImGui::InputText("some input", someInput, sizeof(char) * 128);
    if (ImGui::Button("Button"))
    {
      
    }
  }
  ImGui::End();
}