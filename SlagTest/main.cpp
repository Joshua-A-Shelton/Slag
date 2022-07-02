#include <iostream>
#include <fstream>
#include "../src/SlagLib.h"
#include "../src/FrameBuffer.h"
#include "../src/Surface.h"
#include "../src/Shader.h"
int main()
{
    slag::initialize(true);
    auto count = slag::graphicsCardCount();
    slag::GraphicsCard* card = slag::getGraphicsCard(count-1);
    //auto frameBuffer = slag::FrameBuffer::Create(200,200,card);


    //setup window
    xcb_connection_t   *conn;
    const xcb_setup_t  *setup;
    xcb_screen_t       *screen;
    xcb_window_t       window_id;
    uint32_t           prop_name;
    uint32_t           prop_value;

    /* Connect to X server */
    conn = xcb_connect(NULL, NULL);
    if(xcb_connection_has_error(conn)) {
        printf("Error opening display.\n");
        exit(1);
    }

    /* Obtain setup info and access the screen */
    setup = xcb_get_setup(conn);
    screen = xcb_setup_roots_iterator(setup).data;

    /* Create window */
    window_id = xcb_generate_id(conn);
    prop_name = XCB_CW_BACK_PIXEL;
    prop_value = screen->white_pixel;

    xcb_create_window(conn, screen->root_depth,
                      window_id, screen->root, 0, 0, 500, 500, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, prop_name, &prop_value);

    /* Display the window */
    xcb_map_window(conn, window_id);
    xcb_flush(conn);


    //create surface
    auto surface = slag::Surface(conn,window_id,card, true,slag::Surface::RenderingMode::doubleBuffer);
    auto renderer = surface.getRenderer();
    bool keeprendering = true;
    uint32_t frame = 0;

    float red = 0;

    std::ifstream file("/home/josh/CLionProjects/SlagLib/SlagLib/SlagTest/Shaders/triangle.vert.spv", std::ios::ate | std::ios::binary);
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> vertexData(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read((char*)vertexData.data(), fileSize);
    file.close();

    std::ifstream file2("/home/josh/CLionProjects/SlagLib/SlagLib/SlagTest/Shaders/triangle.frag.spv", std::ios::ate | std::ios::binary);
    fileSize = (size_t)file2.tellg();
    std::vector<uint32_t> fragData(fileSize / sizeof(uint32_t));
    file2.seekg(0);
    file2.read((char*)fragData.data(), fileSize);
    file2.close();


    slag::Shader triangleShader(card,vertexData.data(),vertexData.size(),"main",fragData.data(),fragData.size(),"main",surface.defaultFramebuffer());
    while(keeprendering)
    {
        surface.resizeToWindow();
        renderer->beginFrame();
        renderer->bindDefaultFrameBuffer();
        renderer->bindShader(triangleShader);
        renderer->draw(3,1,0,0);
        renderer->unBindCurrentFrameBuffer();
        renderer->endFrame();
        if(red>1)
        {
            red=0;
        }
        surface.setClearColor(0,0,red,1);
        red+=.01;
        //std::cout << frame++ << "\n";
    }

    //frameBuffer->destroy();

    slag::destroy();
    /* Disconnect from X server */
    xcb_disconnect(conn);
    int i=0;
}