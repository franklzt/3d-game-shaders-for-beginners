/*
  (C) 2019 David Lettier
  lettier.com
*/

#version 150
uniform mat4 p3d_ModelViewMatrix;
in vec4 p3d_Vertex;

void main() {
  vertexPosition = p3d_ModelViewMatrix * p3d_Vertex;
  gl_Position = vertexPosition;
}
