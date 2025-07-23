#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <glm/glm.hpp>            // GLM cho vector, ma trận
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>   // Đọc con trỏ ma trận

// Kích thước màn hình và thông tin
GLFWwindow* window;
GLFWmonitor* monitor;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Màu sắc (GLM vector)
glm::vec3 WHITE(1.0f, 1.0f, 1.0f);
glm::vec3 GREEN(0.0f, 1.0f, 0.0f);
glm::vec3 BLACK(0.0f, 0.0f, 0.0f);
glm::vec3 RED(1.0f, 0.0f, 0.0f);

// FreeType
FT_Library ft;
FT_Face face;

// Lưu trữ glyphs
struct Character {
    GLuint TextureID;
    glm::ivec2 Size;      // Vector 2D lưu kích thước glyph
    glm::ivec2 Bearing;   // Vector 2D lưu khoảng cách từ gốc tới glyph
    GLuint Advance;
};
std::map<GLchar, Character> Characters;

// Hàm khởi tạo OpenGL
void initOpenGL() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        exit(-1);
    }

    monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    SCREEN_WIDTH = mode->width;
    SCREEN_HEIGHT = mode->height;

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vietnam Fighters Battlegrounds", monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create fullscreen window!" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        exit(-1);
    }

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

// Hàm khởi tạo FreeType
void initFreeType() {
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not initialize FreeType library!" << std::endl;
        exit(-1);
    }
    if (FT_New_Face(ft, "path/to/your/font.ttf", 0, &face)) {
        std::cerr << "Failed to load font!" << std::endl;
        exit(-1);
    }
    FT_Set_Pixel_Sizes(face, 0, 48);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph: " << c << std::endl;
            continue;
        }
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x >> 6)
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

// Hàm vẽ văn bản
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor3f(color.r, color.g, color.b);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (auto c : text) {
        Character ch = Characters[c];
        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(xpos, ypos);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(xpos + w, ypos);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(xpos + w, ypos + h);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(xpos, ypos + h);
        glEnd();

        x += (ch.Advance >> 6) * scale;
    }
}

// Kiểm tra người chơi lần đầu
bool isFirstTimePlayer() {
    std::ifstream file("userdata.txt");
    return !file.is_open();
}

// Lưu tài khoản người chơi
void registerPlayer(const std::string& username, const std::string& password) {
    std::ofstream file("userdata.txt");
    file << username << std::endl << password << std::endl;
    file.close();
}

// Xác thực tài khoản
bool authenticatePlayer(const std::string& username, const std::string& password) {
    std::ifstream file("userdata.txt");
    std::string savedUsername, savedPassword;
    file >> savedUsername >> savedPassword;
    return (username == savedUsername && password == savedPassword);
}

// Màn hình đăng ký
void showRegistrationScreen() {
    std::string username, password;
    renderText("Register", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, 1.0f, GREEN);
    std::cout << "Tên đăng nhập: ";
    std::cin >> username;
    std::cout << "Mật khẩu: ";
    std::cin >> password;

    registerPlayer(username, password);
    renderText("Đăng ký thành công!", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 50, 1.0f, GREEN);
}

// Màn hình đăng nhập
void showLoginScreen() {
    std::string username, password;
    renderText("Login", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, 1.0f, GREEN);
    std::cout << "Tên đăng nhập: ";
    std::cin >> username;
    std::cout << "Mật khẩu: ";
    std::cin >> password;

    if (authenticatePlayer(username, password)) {
        renderText("Đăng nhập thành công!", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 50, 1.0f, GREEN);
    } else {
        renderText("Sai thông tin đăng nhập!", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 50, 1.0f, RED);
    }
}

// Vòng lặp chính
void mainLoop() {
    if (isFirstTimePlayer()) {
        showRegistrationScreen();
    } else {
        showLoginScreen();
    }

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        renderText("Vietnam Fighters Battlegrounds", SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4, 1.0f, BLACK);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

// Dọn dẹp tài nguyên
void cleanup() {
    for (auto& c : Characters) {
        glDeleteTextures(1, &c.second.TextureID);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

// Hàm chính
int main() {
    initOpenGL();
    initFreeType();
    mainLoop();
    cleanup();
    return 0;
}
