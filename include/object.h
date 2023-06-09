#pragma once

#include "shader.h"

class ObjectGL final
{
public:
   enum LayoutLocation { VertexLoc = 0, NormalLoc, TextureLoc };

   ObjectGL();
   ~ObjectGL();

   void setEmissionColor(const glm::vec4& emission_color);
   void setAmbientReflectionColor(const glm::vec4& ambient_reflection_color);
   void setDiffuseReflectionColor(const glm::vec4& diffuse_reflection_color);
   void setSpecularReflectionColor(const glm::vec4& specular_reflection_color);
   void setSpecularReflectionExponent(const float& specular_reflection_exponent);
   void setObject(GLenum draw_mode, const std::vector<glm::vec3>& vertices);
   void setObject(
      GLenum draw_mode,
      const std::vector<glm::vec3>& vertices,
      const std::vector<glm::vec3>& normals
   );
   void setObject(
      GLenum draw_mode,
      const std::vector<glm::vec3>& vertices,
      const std::vector<glm::vec2>& textures,
      const std::string& texture_file_path,
      bool is_grayscale = false
   );
   void setObject(
      GLenum draw_mode,
      const std::vector<glm::vec3>& vertices,
      const std::vector<glm::vec3>& normals,
      const std::vector<glm::vec2>& textures
   );
   void setObject(
      GLenum draw_mode,
      const std::vector<glm::vec3>& vertices,
      const std::vector<glm::vec3>& normals,
      const std::vector<glm::vec2>& textures,
      const std::string& texture_file_path,
      bool is_grayscale = false
   );
   void setObject(GLenum draw_mode, const std::string& obj_file_path);
   void setObject(
      GLenum draw_mode,
      const std::string& obj_file_path,
      const std::string& texture_file_name
   );
   void setSquareObject(GLenum draw_mode, bool use_texture = true);
   void setSquareObject(
      GLenum draw_mode,
      const std::string& texture_file_path,
      bool is_grayscale = false
   );
   int addTexture(const std::string& texture_file_path, bool is_grayscale = false);
   void addTexture(int width, int height, bool is_grayscale = false);
   int addTexture(const uint8_t* image_buffer, int width, int height, bool is_grayscale = false);
   void transferUniformsToShader(const ShaderGL* shader);
   void updateDataBuffer(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals);
   void updateDataBuffer(
      const std::vector<glm::vec3>& vertices,
      const std::vector<glm::vec3>& normals,
      const std::vector<glm::vec2>& textures
   );
   void replaceVertices(const std::vector<glm::vec3>& vertices, bool normals_exist, bool textures_exist);
   void replaceVertices(const std::vector<float>& vertices, bool normals_exist, bool textures_exist);
   [[nodiscard]] bool isAdjacencyMode() const { return AdjacencyMode; }
   [[nodiscard]] GLuint getVAO() const { return VAO; }
   [[nodiscard]] GLuint getIBO() const { return IBO; }
   [[nodiscard]] GLenum getDrawMode() const { return DrawMode; }
   [[nodiscard]] GLsizei getVertexNum() const { return VerticesCount; }
   [[nodiscard]] GLsizei getIndexNum() const { return static_cast<GLsizei>(IndexBuffer.size()); }
   [[nodiscard]] GLuint getTextureID(int index) const { return TextureID[index]; }
   [[nodiscard]] int getTextureNum() const { return static_cast<int>(TextureID.size()); }

   template<typename T>
   void addShaderStorageBufferObject(const std::string& name, GLuint binding_index, int data_size)
   {
      GLuint buffer;
      glCreateBuffers( 1, &buffer );
      glBindBufferBase( GL_SHADER_STORAGE_BUFFER, binding_index, buffer );
      glBufferStorage( GL_SHADER_STORAGE_BUFFER, sizeof( T ) * data_size, nullptr, GL_DYNAMIC_DRAW );
      CustomBuffers[name] = buffer;
   }

   template<typename T>
   void addCustomBufferObject(
      const std::string& name,
      GLenum target,
      const std::vector<T>& data,
      GLenum usage
   )
   {
      GLuint buffer;
      glCreateBuffers( 1, &buffer );
      glBindBuffer( target, buffer );
      glBufferStorage( target, sizeof( T ) * data.size(), data.data(), usage );
      CustomBuffers[name] = buffer;
   }

   template<typename T>
   void updateCustomBufferObject(const std::string& name, const std::vector<T>& data)
   {
      const auto it = CustomBuffers.find( name );
      if (it == CustomBuffers.end()) return;

      glNamedBufferSubData( it->second, 0, sizeof( T ) * data.size(), data.data() );
   }

private:
   struct VectorComparison
   {
      [[nodiscard]] bool operator()(const glm::vec3& a, const glm::vec3& b) const
      {
         if (a.x < b.x) return true;
         else if (a.x == b.x) {
            if (a.y < b.y) return true;
            else if (a.y == b.y) if (a.z < b.z) return true;
         }
         return false;
      }
   };

   bool AdjacencyMode;
   GLuint VAO;
   GLuint VBO;
   GLuint IBO;
   GLenum DrawMode;
   GLsizei VerticesCount;
   std::vector<GLuint> TextureID;
   std::vector<GLfloat> DataBuffer;
   std::vector<GLuint> IndexBuffer;
   std::map<std::string, GLuint> CustomBuffers;
   glm::vec4 EmissionColor;
   glm::vec4 AmbientReflectionColor; // It is usually set to the same color with DiffuseReflectionColor.
                                     // Otherwise, it should be in balance with DiffuseReflectionColor.
   glm::vec4 DiffuseReflectionColor; // the intrinsic color
   glm::vec4 SpecularReflectionColor;
   float SpecularReflectionExponent;

   [[nodiscard]] bool prepareTexture2DUsingFreeImage(const std::string& file_path, bool is_grayscale) const;
   void prepareNormal() const;
   void prepareTexture(bool normals_exist) const;
   void prepareVertexBuffer(int n_bytes_per_vertex);
   void prepareIndexBuffer();
   static void getSquareObject(
      std::vector<glm::vec3>& vertices,
      std::vector<glm::vec3>& normals,
      std::vector<glm::vec2>& textures
   );
   static void findNormals(
      std::vector<glm::vec3>& normals,
      const std::vector<glm::vec3>& vertices,
      const std::vector<GLuint>& vertex_indices
   );
   void findAdjacency(const std::vector<glm::vec3>& vertices, const std::vector<GLuint>& indices);
   [[nodiscard]] bool readObjectFile(
      std::vector<glm::vec3>& vertices,
      std::vector<glm::vec3>& normals,
      std::vector<glm::vec2>& textures,
      const std::string& file_path
   );
};