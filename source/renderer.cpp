#include "renderer.h"

RendererGL::RendererGL() :
   Window( nullptr ), Pause( false ), FrameWidth( 1920 ), FrameHeight( 1080 ), ActiveLightIndex( 0 ),
   Algorithm( ALGORITHM::Z_FAIL ), ClickedPoint( -1, -1 ), Texter( std::make_unique<TextGL>() ),
   MainCamera( std::make_unique<CameraGL>() ), TextCamera( std::make_unique<CameraGL>() ),
   TextShader( std::make_unique<ShaderGL>() ), ShadowVolumeShader( std::make_unique<ShaderGL>() ),
   SceneShader( std::make_unique<ShaderGL>() ), WallObject( std::make_unique<ObjectGL>() ),
   BunnyObject( std::make_unique<ObjectGL>() ), Lights( std::make_unique<LightGL>() )
{
   Renderer = this;

   initialize();
   printOpenGLInformation();
}

RendererGL::~RendererGL()
{
}

void RendererGL::printOpenGLInformation()
{
   std::cout << "****************************************************************\n";
   std::cout << " - GLFW version supported: " << glfwGetVersionString() << "\n";
   std::cout << " - OpenGL renderer: " << glGetString( GL_RENDERER ) << "\n";
   std::cout << " - OpenGL version supported: " << glGetString( GL_VERSION ) << "\n";
   std::cout << " - OpenGL shader version supported: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n";
   std::cout << "****************************************************************\n\n";
}

void RendererGL::initialize()
{
   if (!glfwInit()) {
      std::cout << "Cannot Initialize OpenGL...\n";
      return;
   }
   glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
   glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
   glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );
   glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
   glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

   Window = glfwCreateWindow( FrameWidth, FrameHeight, "Main Camera", nullptr, nullptr );
   glfwMakeContextCurrent( Window );

   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
   }

   registerCallbacks();

   glEnable( GL_DEPTH_TEST );
   glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );

   Texter->initialize();

   TextCamera->update2DCamera( FrameWidth, FrameHeight );
   MainCamera->updatePerspectiveCamera( FrameWidth, FrameHeight );

   const std::string shader_directory_path = std::string(CMAKE_SOURCE_DIR) + "/shaders";
   TextShader->setShader(
      std::string(shader_directory_path + "/text.vert").c_str(),
      std::string(shader_directory_path + "/text.frag").c_str()
   );
   ShadowVolumeShader->setShader(
      std::string(shader_directory_path + "/shadow_volume.vert").c_str(),
      std::string(shader_directory_path + "/shadow_volume.frag").c_str(),
      std::string(shader_directory_path + "/shadow_volume.geom").c_str()
   );
   SceneShader->setShader(
      std::string(shader_directory_path + "/scene_shader.vert").c_str(),
      std::string(shader_directory_path + "/scene_shader.frag").c_str()
   );
}

void RendererGL::writeFrame(const std::string& name) const
{
   const int size = FrameWidth * FrameHeight * 3;
   auto* buffer = new uint8_t[size];
   glPixelStorei( GL_PACK_ALIGNMENT, 1 );
   glReadBuffer( GL_COLOR_ATTACHMENT0 );
   glReadPixels( 0, 0, FrameWidth, FrameHeight, GL_BGR, GL_UNSIGNED_BYTE, buffer );
   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, FrameWidth, FrameHeight, FrameWidth * 3, 24,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, name.c_str() );
   FreeImage_Unload( image );
   delete [] buffer;
}

void RendererGL::writeDepthTexture(const std::string& name) const
{
   const int size = FrameWidth * FrameHeight;
   auto* buffer = new uint8_t[size];
   auto* raw_buffer = new GLfloat[size];
   glPixelStorei( GL_PACK_ALIGNMENT, 1 );
   glReadBuffer( GL_DEPTH_ATTACHMENT );
   glReadPixels( 0, 0, FrameWidth, FrameHeight, GL_DEPTH_COMPONENT, GL_FLOAT, raw_buffer );
   for (int i = 0; i < size; ++i) {
      buffer[i] = static_cast<uint8_t>(MainCamera->linearizeDepthValue( raw_buffer[i] ) * 255.0f);
   }
   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, FrameWidth, FrameHeight, FrameWidth, 8,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, name.c_str() );
   FreeImage_Unload( image );
   delete [] raw_buffer;
   delete [] buffer;
}

void RendererGL::writeStencilTexture(const std::string& name) const
{
   const int size = FrameWidth * FrameHeight;
   auto* buffer = new uint8_t[size];
   auto* raw_buffer = new uint32_t[size];
   glPixelStorei( GL_PACK_ALIGNMENT, 1 );
   glReadBuffer( GL_STENCIL_ATTACHMENT );
   glReadPixels( 0, 0, FrameWidth, FrameHeight, GL_STENCIL_INDEX, GL_UNSIGNED_INT, raw_buffer );
   for (int i = 0 ; i < size; ++i) buffer[i] = raw_buffer[i] == 0 ? 255 : 0;
   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, FrameWidth, FrameHeight, FrameWidth, 8,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, name.c_str() );
   FreeImage_Unload( image );
   delete [] raw_buffer;
   delete [] buffer;
}

void RendererGL::cleanup(GLFWwindow* window)
{
   glfwSetWindowShouldClose( window, GLFW_TRUE );
}

void RendererGL::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (action != GLFW_PRESS) return;

   switch (key) {
      case GLFW_KEY_1:
         Renderer->Algorithm = ALGORITHM::Z_FAIL;
         std::cout << "Z-Fail Algorithm Selected\n";
         break;
      case GLFW_KEY_2:
         Renderer->Algorithm = ALGORITHM::Z_PASS;
         std::cout << "Z-Pass Algorithm Selected\n";
         break;
      case GLFW_KEY_3:
         Renderer->Algorithm = ALGORITHM::GPU_GEMS3_CH11;
         break;
      case GLFW_KEY_C:
         Renderer->writeFrame( "../result.png" );
         break;
      case GLFW_KEY_L:
         Renderer->Lights->toggleLightSwitch();
         std::cout << "Light Turned " << (Renderer->Lights->isLightOn() ? "On!\n" : "Off!\n");
         break;
      case GLFW_KEY_P: {
         const glm::vec3 pos = Renderer->MainCamera->getCameraPosition();
         std::cout << "Camera Position: " << pos.x << ", " << pos.y << ", " << pos.z << "\n";
      } break;
      case GLFW_KEY_SPACE:
         Renderer->Pause = !Renderer->Pause;
         break;
      case GLFW_KEY_Q:
      case GLFW_KEY_ESCAPE:
         cleanup( window );
         break;
      default:
         return;
   }
}

void RendererGL::cursor(GLFWwindow* window, double xpos, double ypos)
{
   if (Renderer->Pause) return;

   if (Renderer->MainCamera->getMovingState()) {
      const auto x = static_cast<int>(std::round( xpos ));
      const auto y = static_cast<int>(std::round( ypos ));
      const int dx = x - Renderer->ClickedPoint.x;
      const int dy = y - Renderer->ClickedPoint.y;
      Renderer->MainCamera->moveForward( -dy );
      Renderer->MainCamera->rotateAroundWorldY( -dx );

      if (glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS) {
         Renderer->MainCamera->pitch( -dy );
      }

      Renderer->ClickedPoint.x = x;
      Renderer->ClickedPoint.y = y;
   }
}

void RendererGL::mouse(GLFWwindow* window, int button, int action, int mods)
{
   if (Renderer->Pause) return;

   if (button == GLFW_MOUSE_BUTTON_LEFT) {
      const bool moving_state = action == GLFW_PRESS;
      if (moving_state) {
         double x, y;
         glfwGetCursorPos( window, &x, &y );
         Renderer->ClickedPoint.x = static_cast<int>(std::round( x ));
         Renderer->ClickedPoint.y = static_cast<int>(std::round( y ));
      }
      Renderer->MainCamera->setMovingState( moving_state );
   }
}

void RendererGL::mousewheel(GLFWwindow* window, double xoffset, double yoffset)
{
   if (Renderer->Pause) return;

   if (yoffset >= 0.0) Renderer->MainCamera->zoomIn();
   else Renderer->MainCamera->zoomOut();
}

void RendererGL::registerCallbacks() const
{
   glfwSetWindowCloseCallback( Window, cleanup );
   glfwSetKeyCallback( Window, keyboard );
   glfwSetCursorPosCallback( Window, cursor );
   glfwSetMouseButtonCallback( Window, mouse );
   glfwSetScrollCallback( Window, mousewheel );
}

void RendererGL::setLights() const
{
   const glm::vec4 light_position(500.0f, 500.0f, 500.0f, 1.0f);
   const glm::vec4 ambient_color(1.0f, 1.0f, 1.0f, 1.0f);
   const glm::vec4 diffuse_color(0.9f, 0.9f, 0.9f, 1.0f);
   const glm::vec4 specular_color(0.9f, 0.9f, 0.9f, 1.0f);
   Lights->addLight( light_position, ambient_color, diffuse_color, specular_color );
}

void RendererGL::setWallObject() const
{
   constexpr float half_length = 128.0f;
   std::vector<glm::vec3> wall_vertices;
   wall_vertices.emplace_back( half_length, 0.0f, half_length );
   wall_vertices.emplace_back( half_length, 0.0f, -half_length );
   wall_vertices.emplace_back( -half_length, 0.0f, -half_length );

   wall_vertices.emplace_back( -half_length, 0.0f, half_length );
   wall_vertices.emplace_back( half_length, 0.0f, half_length );
   wall_vertices.emplace_back( -half_length, 0.0f, -half_length );

   std::vector<glm::vec3> wall_normals;
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   WallObject->setObject( GL_TRIANGLES, wall_vertices, wall_normals );
   WallObject->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
}

void RendererGL::setBunnyObject() const
{
   const std::string sample_directory_path = std::string(CMAKE_SOURCE_DIR) + "/samples";
   BunnyObject->setObject(
      GL_TRIANGLES_ADJACENCY,
      std::string(sample_directory_path + "/Bunny/bunny.obj")
   );
   BunnyObject->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
}

void RendererGL::getBoundingBox(std::array<glm::vec3, 8>& bounding_box, const std::array<glm::vec3, 8>& points)
{
   auto min_point = glm::vec3(std::numeric_limits<float>::max());
   auto max_point = glm::vec3(std::numeric_limits<float>::lowest());
   for (int i = 0; i < 8; ++i) {
      if (points[i].x < min_point.x) min_point.x = points[i].x;
      if (points[i].y < min_point.y) min_point.y = points[i].y;
      if (points[i].z < min_point.z) min_point.z = points[i].z;

      if (points[i].x > max_point.x) max_point.x = points[i].x;
      if (points[i].y > max_point.y) max_point.y = points[i].y;
      if (points[i].z > max_point.z) max_point.z = points[i].z;
   }

   bounding_box[0] = glm::vec3(min_point.x, min_point.y, min_point.z);
   bounding_box[1] = glm::vec3(max_point.x, min_point.y, min_point.z);
   bounding_box[2] = glm::vec3(min_point.x, min_point.y, max_point.z);
   bounding_box[3] = glm::vec3(max_point.x, min_point.y, max_point.z);
   bounding_box[4] = glm::vec3(min_point.x, max_point.y, min_point.z);
   bounding_box[5] = glm::vec3(max_point.x, max_point.y, min_point.z);
   bounding_box[6] = glm::vec3(min_point.x, max_point.y, max_point.z);
   bounding_box[7] = glm::vec3(max_point.x, max_point.y, max_point.z);
}

void RendererGL::drawBoxObject(ShaderGL* shader, const CameraGL* camera) const
{
   glm::mat4 to_world(1.0f);
   shader->transferBasicTransformationUniforms( to_world, camera );
   WallObject->setDiffuseReflectionColor( { 0.0f, 0.0f, 1.0f, 1.0f } );
   WallObject->transferUniformsToShader( shader );
   glBindVertexArray( WallObject->getVAO() );
   glDrawArrays( WallObject->getDrawMode(), 0, WallObject->getVertexNum() );

   to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(0.0f, 128.0f, -128.0f) ) *
      glm::rotate( glm::mat4(1.0f), glm::radians( 90.0f ), glm::vec3(1.0f, 0.0f, 0.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera );
   WallObject->setDiffuseReflectionColor( { 0.0f, 1.0f, 0.0f, 1.0f } );
   WallObject->transferUniformsToShader( shader );
   glDrawArrays( WallObject->getDrawMode(), 0, WallObject->getVertexNum() );

   to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(-128.0f, 128.0f, 0.0f) ) *
      glm::rotate( glm::mat4(1.0f), glm::radians( -90.0f ), glm::vec3(0.0f, 0.0f, 1.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera );
   WallObject->setDiffuseReflectionColor( { 1.0f, 0.0f, 0.0f, 1.0f } );
   WallObject->transferUniformsToShader( shader );
   glDrawArrays( WallObject->getDrawMode(), 0, WallObject->getVertexNum() );
}

void RendererGL::drawBunnyObject(ShaderGL* shader, const CameraGL* camera) const
{
   const glm::mat4 to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, -30.0f) ) *
      glm::scale( glm::mat4(1.0f), glm::vec3(100.0f, 100.0f, 100.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera );
   BunnyObject->transferUniformsToShader( shader );

   glBindVertexArray( BunnyObject->getVAO() );
   if (!BunnyObject->isAdjacencyMode()) glDrawArrays( BunnyObject->getDrawMode(), 0, BunnyObject->getVertexNum() );
   else {
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, BunnyObject->getIBO() );
      glDrawElements( BunnyObject->getDrawMode(), BunnyObject->getIndexNum(), GL_UNSIGNED_INT, nullptr );
   }
}

void RendererGL::drawDepthMap() const
{
   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glDepthFunc( GL_LESS );
   glDrawBuffer( GL_NONE );
   glUseProgram( SceneShader->getShaderProgram() );
   drawBunnyObject( SceneShader.get(), MainCamera.get() );
   drawBoxObject( SceneShader.get(), MainCamera.get() );
}

void RendererGL::drawShadowVolumeWithZFail() const
{
   // Need to do the depth test, but do not write the result.
   glDepthMask( GL_FALSE );

   // Do not near/far plane clipping due to projection-to-infinity.
   glEnable( GL_DEPTH_CLAMP );

   // All the front- or back-facing facets should be rendered to generate the shadow volume.
   glDisable( GL_CULL_FACE );

   // Only the depth test matters. (test order: stencil -> depth)
   glStencilFunc( GL_ALWAYS, 0, ~0 );

   glStencilOpSeparate( GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP );
   glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP );

   glUseProgram( ShadowVolumeShader->getShaderProgram() );
   const glm::vec4 light_position_in_eye = MainCamera->getViewMatrix() * Lights->getLightPosition( 0 );
   ShadowVolumeShader->uniform3fv( "LightPosition", glm::vec3(light_position_in_eye) );
   drawBunnyObject( ShadowVolumeShader.get(), MainCamera.get() );

   glDepthMask( GL_TRUE );
   glDisable( GL_DEPTH_CLAMP );
   glEnable( GL_CULL_FACE );
}

void RendererGL::drawShadowVolumeWithZPass() const
{
   // Need to do the depth test, but do not write the result.
   glDepthMask( GL_FALSE );

   // Do not near/far plane clipping due to projection-to-infinity.
   glEnable( GL_DEPTH_CLAMP );

   // All the front- or back-facing facets should be rendered to generate the shadow volume.
   glDisable( GL_CULL_FACE );

   // Only the depth test matters. (test order: stencil -> depth)
   glStencilFunc( GL_ALWAYS, 0, ~0 );

   glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP );
   glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP );

   glUseProgram( ShadowVolumeShader->getShaderProgram() );
   const glm::vec4 light_position_in_eye = MainCamera->getViewMatrix() * Lights->getLightPosition( 0 );
   ShadowVolumeShader->uniform3fv( "LightPosition", glm::vec3(light_position_in_eye) );
   drawBunnyObject( ShadowVolumeShader.get(), MainCamera.get() );

   glDepthMask( GL_TRUE );
   glDisable( GL_DEPTH_CLAMP );
   glEnable( GL_CULL_FACE );
}

void RendererGL::drawShadow() const
{
   // GL_BACK is the initial value for double-buffered contexts.
   glDrawBuffer( GL_BACK );

   glStencilFunc( GL_EQUAL, 0, 0xFF );
   glStencilOpSeparate( GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP );

   glDepthFunc( GL_EQUAL );

   glUseProgram( SceneShader->getShaderProgram() );
   Lights->transferUniformsToShader( SceneShader.get() );
   glUniform1i( SceneShader->getLocation( "LightIndex" ), ActiveLightIndex );
   glUniform1i( SceneShader->getLocation( "UseTexture" ), 0 );
   drawBunnyObject( SceneShader.get(), MainCamera.get() );
   drawBoxObject( SceneShader.get(), MainCamera.get() );
}

void RendererGL::drawText(const std::string& text, glm::vec2 start_position) const
{
   std::vector<TextGL::Glyph*> glyphs;
   Texter->getGlyphsFromText( glyphs, text );

   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( TextShader->getShaderProgram() );

   glEnable( GL_BLEND );
   glBlendFunc( GL_SRC_ALPHA, GL_ONE );
   glDisable( GL_DEPTH_TEST );

   glm::vec2 text_position = start_position;
   const ObjectGL* glyph_object = Texter->getGlyphObject();
   glBindVertexArray( glyph_object->getVAO() );
   for (const auto& glyph : glyphs) {
      const glm::vec2 position(
         std::round( text_position.x + glyph->Bearing.x ),
         std::round( text_position.y + glyph->Bearing.y - glyph->Size.y )
      );
      const glm::mat4 to_world =
         glm::translate( glm::mat4(1.0f), glm::vec3(position, 0.0f) ) *
         glm::scale( glm::mat4(1.0f), glm::vec3(glyph->Size.x, glyph->Size.y, 1.0f) );
      TextShader->transferBasicTransformationUniforms( to_world, TextCamera.get() );
      TextShader->uniform2fv( "TextScale", glyph->TopRightTextureCoord );
      glBindTextureUnit( 0, glyph_object->getTextureID( glyph->TextureIDIndex ) );
      glDrawArrays( glyph_object->getDrawMode(), 0, glyph_object->getVertexNum() );

      text_position.x += glyph->Advance.x;
      text_position.y -= glyph->Advance.y;
   }
   glEnable( GL_DEPTH_TEST );
   glDisable( GL_BLEND );
}

void RendererGL::render() const
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

   std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

   drawDepthMap();
   //writeDepthTexture( "../depth.png" );

   glEnable( GL_STENCIL_TEST );
   switch (Algorithm) {
      case ALGORITHM::Z_FAIL: drawShadowVolumeWithZFail(); break;
      case ALGORITHM::Z_PASS: drawShadowVolumeWithZPass(); break;
   }

   //writeStencilTexture( "../stencil.png" );
   drawShadow();
   glDisable( GL_STENCIL_TEST );

   std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
   const auto fps = 1E+6 / static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
   std::stringstream text;
   text << std::fixed << std::setprecision( 2 ) << fps << " fps";
   drawText( text.str(), { 50.0f, 1000.0f } );
}

void RendererGL::play()
{
   if (glfwWindowShouldClose( Window )) initialize();

   setLights();
   setWallObject();
   setBunnyObject();

   TextShader->setTextUniformLocations();
   SceneShader->setSceneUniformLocations( 1 );
   ShadowVolumeShader->setShadowVolumeUniformLocations();

   while (!glfwWindowShouldClose( Window )) {
      if (!Pause) render();

      glfwSwapBuffers( Window );
      glfwPollEvents();
   }
   glfwDestroyWindow( Window );
}