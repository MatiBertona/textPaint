#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>q
#include "Callbacks.hpp"
#include "Debug.hpp"
#include "Model.hpp"
#include "Shaders.hpp"
#include "Window.hpp"
#define VERSION 20250901

Window main_window; // ventana principal: muestra el modelo en 3d sobre el que se pinta
Window aux_window; // ventana auxiliar que muestra la textura

void drawMain(); // dibuja el modelo "normalmente" para la ventana principal
void drawBack(); // dibuja el modelo con un shader alternativo para convertir coords de la ventana a coords de textura
void drawAux(); // dibuja la textura en la ventana auxiliar
void drawImGui(Window &window); // settings sub-window

float radius = 5; // radio del "pincel" con el que pintamos en la textura
glm::vec4 color = { 0.f, 0.f, 0.f, 1.f }; // color actual con el que se pinta en la textura
int prev_col = -1, prev_row = -1; // Coordenadas ANTERIORES en la imagen
Texture texture; // textura (compartida por ambas ventanas)
Image image; // imagen (para la textura, Image est� en RAM, Texture la env�a a GPU)
Image capa_trazo;   // Nuestro "lienzo auxiliar" o "capa temporal"

bool pintando = false; // Un 'flag' para saber si estamos en medio de un trazo
Model model_chookity; // el objeto a pintar, para renderizar en la ventan principal
Model model_aux; // un quad para cubrir la ventana auxiliar y mostrar la textura

Shader shader_main; // shader para el objeto principal (drawMain)
Shader shader_aux; // shader para la ventana auxiliar (drawTexture)

// callbacks del mouse y auxiliares para los callbacks
enum class MouseAction { None, ManipulateView, Draw };
MouseAction mouse_action = MouseAction::None; // qu� hacer en el callback del motion si el bot�n del mouse est� apretado
void mainMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void mainMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void auxMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void auxMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);


// Funci�n para mezclar colores con alpha blending
glm::vec3 alphaMezcla(const glm::vec3& newColor, const glm::vec3& oldColor, float alpha) {
	return newColor * alpha + oldColor * (1.0f - alpha);
}
// NOTA: Para esto, tu clase Image necesita manejar RGBA. 
// Si solo ten�s GetRGB/SetRGB, tendr�amos que adaptarla. �Avisame!
void paintRadioEnCapa(Image& capa, int center_x, int center_y, float rad, const glm::vec4& brush_color) {
	for (int y = center_y - rad; y <= center_y + rad; y++) {
		for (int x = center_x - rad; x <= center_x + rad; x++) {
			if (y < 0 || y >= capa.GetHeight() || x < 0 || x >= capa.GetWidth()) continue;
			
			float dist = glm::distance(glm::vec2(x, y), glm::vec2(center_x, center_y));
			
			if (dist < rad) {
				float alpha_pixel = 1.0f - (dist / rad);
				float alpha_final = alpha_pixel * brush_color.a;
				
				// Leemos el alpha que YA EST� en la capa temporal
				float alpha_actual_en_capa = capa.GetRGBA(y, x).a;
				
				// Solo pintamos si nuestro nuevo alpha es MAYOR.
				// As�, el trazo se forma con sus partes m�s opacas.
				if (alpha_final > alpha_actual_en_capa) {
					capa.SetRGBA(y, x, glm::vec4(glm::vec3(brush_color), alpha_final));
				}
			}
		}
	}
}
// Dibuja un c�rculo con anti-aliasing en la imagen
void paintRadio(Image& img, int center_x, int center_y, float rad, const glm::vec4& brush_color) {
	// Iteramos sobre la caja que contiene al c�rculo
	for (int y = center_y - rad; y <= center_y + rad; y++) {
		for (int x = center_x - rad; x <= center_x + rad; x++) {
			
			if (y < 0 || y >= img.GetHeight() || x < 0 || x >= img.GetWidth()) continue;
			
			float dist = glm::distance(glm::vec2(x, y), glm::vec2(center_x, center_y));
			
			if (dist < rad) {
				// El alpha disminuye linealmente desde el centro (1.0) hasta el borde (0.0)
				float alpha_pixel = 1.0f - (dist / rad);
				
				// Combinamos el alpha del pincel con el del degradado
				float alpha_final = alpha_pixel * brush_color.a;
				
				glm::vec3 old_color = img.GetRGB(y, x);
				glm::vec3 final_color = alphaMezcla(glm::vec3(brush_color), old_color, alpha_final);
				img.SetRGB(y, x, final_color);
			}
		}
	}
}
int main() {
	// main window (3D view)
	main_window = Window(800, 600, "Main View", true);
	glfwSetCursorPosCallback(main_window, mainMouseMoveCallback);
	glfwSetMouseButtonCallback(main_window, mainMouseButtonCallback);
	main_window.getCamera().model_angle = 2.5;
	
	glClearColor(1.f,1.f,1.f,1.f);
	shader_main = Shader("shaders/main");
	
	image = Image("models/chookity.png",true);
	texture = Texture(image);
	
	model_chookity = Model::loadSingle("models/chookity", Model::fNoTextures);
	
	// aux window (texture image)
	aux_window = Window(512,512, "Texture", true, main_window);
	glfwSetCursorPosCallback(aux_window, auxMouseMoveCallback);
	glfwSetMouseButtonCallback(aux_window, auxMouseButtonCallback);
	
	model_aux = Model::loadSingle("models/texquad", Model::fNoTextures);
	shader_aux = Shader("shaders/quad");
	
	// main loop
	do {
		glfwPollEvents();
		
		glfwMakeContextCurrent(main_window);
		drawMain();
		drawImGui(main_window);
		glFinish();
		glfwSwapBuffers(main_window);
		
		glfwMakeContextCurrent(aux_window);
		drawAux();
		drawImGui(aux_window);
		glFinish();
		glfwSwapBuffers(aux_window);
		
	} while( (not glfwWindowShouldClose(main_window)) and (not glfwWindowShouldClose(aux_window)) );
}


// ===== pasos del renderizado =====

void drawMain() {
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	texture.bind();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	shader_main.use();
	setMatrixes(main_window, shader_main);
	shader_main.setLight(glm::vec4{-1.f,1.f,4.f,1.f}, glm::vec3{1.f,1.f,1.f}, 0.35f);
	shader_main.setMaterial(model_chookity.material);
	shader_main.setBuffers(model_chookity.buffers);
	model_chookity.buffers.draw();
}

void drawAux() {
	glDisable(GL_DEPTH_TEST);
	texture.bind();
	shader_aux.use();
	shader_aux.setMatrixes(glm::mat4{1.f}, glm::mat4{1.f}, glm::mat4{1.f});
	shader_aux.setBuffers(model_aux.buffers);
	model_aux.buffers.draw();
}

void drawBack() {
	glfwMakeContextCurrent(main_window);
	glDisable(GL_MULTISAMPLE);

	/// @ToDo: Parte 2: renderizar el modelo en 3d con un nuevo shader de forma 
	///                 que queden las coordenadas de textura de cada fragmento
	///                 en el back-buffer de color
	
	glEnable(GL_MULTISAMPLE);
	glFlush();
	glFinish();
}

void drawImGui(Window &window) {
	if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) return;
	// settings sub-window
	window.ImGuiDialog("Settings",[&](){
		ImGui::SliderFloat("Radius",&radius,1,50);
		ImGui::ColorEdit4("Color",&(color[0]),0);
		
		static std::vector<std::pair<const char *, ImVec4>> pallete = { // colores predefindos
			{"white" , {1.f,1.f,1.f,1.f}},
			{"pink"  , {0.749f,0.49f,0.498f,1.f}},
			{"yellow", {0.965f,0.729f,0.106f,1.f}},
			{"black" , {0.f,0.f,0.f,1.f}} };
		
		ImGui::Text("Pallete:");
		for (auto &p : pallete) {
			ImGui::SameLine();
			if (ImGui::ColorButton(p.first, p.second))
				color[0] = p.second.x, color[1] = p.second.y, color[2] = p.second.z;
		}
		
		if (ImGui::Button("Reload Image")) {
			image = Image("models/chookity.png",true);
			texture.update(image);
		}
	});
}

void mapearCoordenadas(GLFWwindow* window, double xpos, double ypos, const Image& image, int& col, int& row) {
	// Obtenemos las dimensiones de la ventana
	int win_width, win_height;
	glfwGetWindowSize(window, &win_width, &win_height);
	col = (xpos / (float)win_width) * image.GetWidth();
	row = ((win_height - ypos) / (float)win_height) * image.GetHeight();
}


// ===== callbacks de la ventana auxiliar (textura) =====
void auxMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
		// --- INICIO DEL TRAZO ---
		pintando = true;
		
		// 1. Creamos nuestra capa temporal, vac�a y transparente
		capa_trazo = Image(image.GetWidth(), image.GetHeight()); 
		
		// 2. Pintamos el primer punto en la capa temporal
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		int col, row;
		mapearCoordenadas(window, xpos, ypos, image, col, row);
		
		// Usaremos una nueva funci�n para pintar solo en la capa
		paintRadioEnCapa(capa_trazo, col, row, radius, color);
		
		prev_col = col;
		prev_row = row;
		
		// IMPORTANTE: No actualizamos la textura final todav�a
		
	} else if (action == GLFW_RELEASE) {
		// --- FIN DEL TRAZO ---
		pintando = false;
		
		// 3. "Aplastamos" la capa_trazo sobre la imagen principal
		for (int y = 0; y < image.GetHeight(); y++) {
			for (int x = 0; x < image.GetWidth(); x++) {
				glm::vec4 color_del_trazo = capa_trazo.GetRGBA(y, x); // Leemos el pixel de la capa
				
				if (color_del_trazo.a > 0) { // Si hay algo pintado en la capa...
					glm::vec3 color_de_fondo = image.GetRGB(y, x);
					glm::vec3 color_final = alphaMezcla(glm::vec3(color_del_trazo), color_de_fondo, color_del_trazo.a);
					image.SetRGB(y, x, color_final);
				}
			}
		}
		
		// 4. Ahora s�, actualizamos la textura en la GPU con el resultado final
		texture.update(image);
		prev_col = -1;
	}

}



void auxMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	if (!pintando) return; // Si no estamos pintando, no hacemos nada
	
	int col, row;
	mapearCoordenadas(window, xpos, ypos, image, col, row);
	
	if (col == prev_col && row == prev_row) return;
	
	// --- Algoritmo DDA sobre la CAPA TEMPORAL ---
	float dx = col - prev_col;
	float dy = row - prev_row;
	float steps = std::max(abs(dx), abs(dy));
	float x_inc = dx / steps;
	float y_inc = dy / steps;
	
	for (int i = 0; i <= steps; i++) {
		int current_col = prev_col + i * x_inc;
		int current_row = prev_row + i * y_inc;
		paintRadioEnCapa(capa_trazo, current_col, current_row, radius, color);
	}
	
	prev_col = col;
	prev_row = row;
	
	// Para que el usuario vea lo que hace en tiempo real, tenemos que mostrarle una vista previa.
	// La forma m�s simple es crear una imagen temporal para la vista, mezclar y actualizar la textura.
	Image preview = image; // Hacemos una copia de la imagen original
	for (int y = 0; y < preview.GetHeight(); y++) {
		for (int x = 0; x < preview.GetWidth(); x++) {
			glm::vec4 color_trazo = capa_trazo.GetRGBA(y, x);
			if (color_trazo.a > 0) {
				glm::vec3 color_fondo = preview.GetRGB(y, x);
				glm::vec3 color_final = alphaMezcla(glm::vec3(color_trazo), color_fondo, color_trazo.a);
				preview.SetRGB(y, x, color_final);
			}
		}
	}
	texture.update(preview); // Actualizamos la GPU con la VISTA PREVIA
}
// ===== callbacks de la ventana principal (vista 3D) =====

void mainMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
	if (action==GLFW_PRESS) {
		if (mods!=0 or button==GLFW_MOUSE_BUTTON_RIGHT) {
			mouse_action = MouseAction::ManipulateView;
			common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
			return;
		}
		
		mouse_action = MouseAction::Draw;
		
		/// @ToDo: Parte 2: pintar un punto de radio "radius" en la imagen
		///                 "image" que se usa como textura
		
	} else {
		if (mouse_action==MouseAction::ManipulateView)
			common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
		mouse_action = MouseAction::None;
	}
}

void mainMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	if (mouse_action!=MouseAction::Draw) {
		if (mouse_action==MouseAction::ManipulateView);
			common_callbacks::mouseMoveCallback(window,xpos,ypos);
		return; 
	}
	
	/// @ToDo: Parte 2: pintar un segmento de ancho "2*radius" en la imagen
	///                 "image" que se usa como textura
	
}
