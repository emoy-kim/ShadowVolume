#version 460

uniform mat4 ProjectionMatrix;
uniform vec3 LightPosition;

layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

const float zero = 0.0f;
const float one = 1.0f;
const float epsilon = 1e-4f;

void emitSideOfVolume(vec3 start, vec3 end)
{
    vec3 light_direction = normalize( start - LightPosition );
    gl_Position = ProjectionMatrix * vec4(start + light_direction * epsilon, one);
    EmitVertex();

    gl_Position = ProjectionMatrix * vec4(light_direction, zero);
    EmitVertex();

    light_direction = normalize( end - LightPosition );
    gl_Position = ProjectionMatrix * vec4(end + light_direction * epsilon, one);
    EmitVertex();

    gl_Position = ProjectionMatrix * vec4(light_direction, zero);
    EmitVertex();
    EndPrimitive();
}

void main()
{
   // six vertices of triangles_adjacency
   // 1 . . . . . . 2 . . . e3 . . .  3
   //  .           / \              .
   //   .         /   \           .
   //    e2      e0    e4       .
   //      .    /       \     .
   //       .  /         \  .
   //        0 --- e1 --- 4
   //         .         .
   //          e5     .
   //            .  .
   //             5
   vec3 e0 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
   vec3 e1 = gl_in[4].gl_Position.xyz - gl_in[0].gl_Position.xyz;
   vec3 e2 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
   vec3 e3 = gl_in[3].gl_Position.xyz - gl_in[2].gl_Position.xyz;
   vec3 e4 = gl_in[4].gl_Position.xyz - gl_in[2].gl_Position.xyz;
   vec3 e5 = gl_in[5].gl_Position.xyz - gl_in[0].gl_Position.xyz;

   vec3 n = cross( e0, e1 );
   vec3 light_direction = LightPosition - gl_in[0].gl_Position.xyz;
   if (dot( n, light_direction ) > epsilon) {
      n = cross( e2, e0 );
      if (dot( n, light_direction ) <= zero) emitSideOfVolume( gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz );

      n = cross( e3, e4 );
      light_direction = LightPosition - gl_in[2].gl_Position.xyz;

      if (dot( n, light_direction ) <= zero) emitSideOfVolume( gl_in[2].gl_Position.xyz, gl_in[4].gl_Position.xyz );

      n = cross( e1, e5 );
      light_direction = LightPosition - gl_in[4].gl_Position.xyz;

      if (dot( n, light_direction ) <= zero) emitSideOfVolume( gl_in[4].gl_Position.xyz, gl_in[0].gl_Position.xyz );

      light_direction = normalize( gl_in[0].gl_Position.xyz - LightPosition );
      gl_Position = ProjectionMatrix * vec4(gl_in[0].gl_Position.xyz + light_direction * epsilon, one);
      EmitVertex();

      light_direction = normalize( gl_in[2].gl_Position.xyz - LightPosition );
      gl_Position = ProjectionMatrix * vec4(gl_in[2].gl_Position.xyz + light_direction * epsilon, one);
      EmitVertex();

      light_direction = normalize( gl_in[4].gl_Position.xyz - LightPosition );
      gl_Position = ProjectionMatrix * vec4(gl_in[4].gl_Position.xyz + light_direction * epsilon, one);
      EmitVertex();
      EndPrimitive();

      gl_Position = ProjectionMatrix * vec4(gl_in[0].gl_Position.xyz - LightPosition, zero);
      EmitVertex();

      gl_Position = ProjectionMatrix * vec4(gl_in[4].gl_Position.xyz - LightPosition, zero);
      EmitVertex();

      gl_Position = ProjectionMatrix * vec4(gl_in[2].gl_Position.xyz - LightPosition, zero);
      EmitVertex();
      EndPrimitive();
   }
}