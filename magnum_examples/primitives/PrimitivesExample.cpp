/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 —
            Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Renderer.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

class PrimitivesExample: public Platform::GlfwApplication {
    public:
        explicit PrimitivesExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        Buffer indexBuffer, positionBuffer, normalBuffer;
        Mesh mesh;
        Shaders::Phong shader;

        Matrix4 transformation, projection;
        Vector2i previousMousePosition;
        Color3 color;
};

PrimitivesExample::PrimitivesExample(const Arguments& arguments):
    Platform::GlfwApplication{arguments, Configuration{}
        .setTitle("Magnum Primitives Example")} {
    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

    const Trade::MeshData3D cube = Primitives::Cube::solid();

    positionBuffer.setData(cube.positions(0), BufferUsage::StaticDraw);

    normalBuffer.setData(cube.normals(0), BufferUsage::StaticDraw);

    indexBuffer.setData(cube.indices(), BufferUsage::StaticDraw);

    mesh.setPrimitive(cube.primitive())
        .setCount(cube.indices().size())
        .addVertexBuffer(positionBuffer, 0, Shaders::Phong::Position{})
        .addVertexBuffer(normalBuffer, 0, Shaders::Phong::Normal{})
        .setIndexBuffer(indexBuffer, 0, Mesh::IndexType::UnsignedInt);

    transformation = Matrix4::rotationX(30.0_degf)
                     * Matrix4::rotationY(40.0_degf);

    color = Color3::fromHsv(35.0_degf, 1.0f, 1.0f);

    projection = Matrix4::perspectiveProjection(35.0_degf,
            Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), 0.01f, 100.0f)
            * Matrix4::translation(Vector3::zAxis(-10.0f));
}

void PrimitivesExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    shader.setLightPosition({7.0f, 5.0f, 2.5f})
        .setLightColor(Color3{1.0f})
        .setDiffuseColor(color)
        .setAmbientColor(Color3::fromHsv(color.hue(), 1.0f, 0.3f))
        .setTransformationMatrix(transformation)
        .setNormalMatrix(transformation.rotationScaling())
        .setProjectionMatrix(projection);
    mesh.draw(shader);

    swapBuffers();
}

void PrimitivesExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    previousMousePosition = event.position();
    event.setAccepted();
}

void PrimitivesExample::mouseReleaseEvent(MouseEvent& event) {
    color = Color3::fromHsv(color.hue() + 50.0_degf, 1.0f, 1.0f);

    event.setAccepted();
    redraw();
}

void PrimitivesExample::mouseMoveEvent(MouseMoveEvent& event) {

    if(glfwGetMouseButton(window(), GLFW_MOUSE_BUTTON_1) != GLFW_PRESS) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - previousMousePosition}/
        Vector2{defaultFramebuffer.viewport().size()};

    transformation =
        Matrix4::rotationX(Rad{delta.y()})*
        transformation*
        Matrix4::rotationY(Rad{delta.x()});

    previousMousePosition = event.position();
    event.setAccepted();
    redraw();
}


int main(int argc, char** argv) {
    PrimitivesExample app({argc, argv});
    return app.exec();
}
