#include <stb_image.h>
#include <stdexcept>
#include <vector>
#include "Image.hpp"
#include "Debug.hpp"

// Constructor que carga desde archivo
Image::Image(const std::string &fname, bool flipY) {
	stbi_set_flip_vertically_on_load(flipY);
	// Forzamos 4 canales (RGBA) para estandarizar
	m_data = stbi_load(fname.c_str(), &m_width, &m_height, &m_channels, 4);
	cg_assert(m_data,"Could not load texture: "+fname);
	m_channels = 4; // Nos aseguramos de que siempre sean 4 canales
}

Image::Image(const Image& other) : m_width(other.m_width), m_height(other.m_height), m_channels(other.m_channels) {
	if (other.m_data) {
		size_t size = m_width * m_height * m_channels;
		m_data = new unsigned char[size];
		std::copy(other.m_data, other.m_data + size, m_data);
	} else {
		m_data = nullptr;
	}
}

Image& Image::operator=(const Image& other) {
	if (this != &other) { // Evitar auto-asignación
		delete[] m_data; // Liberar memoria vieja

		m_width = other.m_width;
		m_height = other.m_height;
		m_channels = other.m_channels;

		if (other.m_data) {
			size_t size = m_width * m_height * m_channels;
			m_data = new unsigned char[size];
			std::copy(other.m_data, other.m_data + size, m_data);
		} else {
			m_data = nullptr;
		}
	}
	return *this;
}
// NUEVO: Constructor para crear una imagen en blanco
Image::Image(int width, int height) {
	m_width = width;
	m_height = height;
	m_channels = 4; // Siempre RGBA
	size_t size = m_width * m_height * m_channels;
	m_data = new unsigned char[size];
	// Inicializamos la imagen a negro transparente
	std::fill(m_data, m_data + size, 0);
}

Image::~Image() {
	if (m_data) stbi_image_free(m_data);
}

Image::Image(Image &&other) noexcept {
	m_data = other.m_data; other.m_data = nullptr;
	m_width = other.m_width;
	m_height = other.m_height;
	m_channels = other.m_channels;
}

Image &Image::operator=(Image &&other) noexcept {
	if (this != &other) {
		if (m_data) stbi_image_free(m_data);
		m_data = other.m_data; other.m_data = nullptr;
		m_width = other.m_width;
		m_height = other.m_height;
		m_channels = other.m_channels;
	}
	return *this;
}

void Image::SetRGB(int i, int j, const glm::vec3 &rgb) {
	check_indexes(i,j);
	int index = (i * m_width + j) * m_channels;
	m_data[index + 0] = static_cast<unsigned char>(glm::clamp(rgb.r, 0.f, 1.f) * 255.f);
	m_data[index + 1] = static_cast<unsigned char>(glm::clamp(rgb.g, 0.f, 1.f) * 255.f);
	m_data[index + 2] = static_cast<unsigned char>(glm::clamp(rgb.b, 0.f, 1.f) * 255.f);
	if(m_channels == 4) m_data[index + 3] = 255; // Opaco por defecto
}

void Image::SetRGBA(int i, int j, const glm::vec4 &rgba) {
	check_indexes(i,j);
	int index = (i * m_width + j) * m_channels;
	m_data[index + 0] = static_cast<unsigned char>(glm::clamp(rgba.r, 0.f, 1.f) * 255.f);
	m_data[index + 1] = static_cast<unsigned char>(glm::clamp(rgba.g, 0.f, 1.f) * 255.f);
	m_data[index + 2] = static_cast<unsigned char>(glm::clamp(rgba.b, 0.f, 1.f) * 255.f);
	if(m_channels == 4) m_data[index + 3] = static_cast<unsigned char>(glm::clamp(rgba.a, 0.f, 1.f) * 255.f);
}

glm::vec3 Image::GetRGB(int i, int j) const {
	check_indexes(i,j);
	int index = (i * m_width + j) * m_channels;
	return { m_data[index + 0] / 255.f,
		m_data[index + 1] / 255.f,
		m_data[index + 2] / 255.f };
}

glm::vec4 Image::GetRGBA(int i, int j) const {
	check_indexes(i,j);
	int index = (i * m_width + j) * m_channels;
	return { m_data[index + 0] / 255.f,
		m_data[index + 1] / 255.f,
		m_data[index + 2] / 255.f,
		m_channels == 4 ? m_data[index + 3] / 255.f : 1.f };
}

void Image::check_indexes(int i, int j) const {
	cg_assert(i >= 0 && i < m_height, "Wrong i (row) coord in Image");
	cg_assert(j >= 0 && j < m_width, "Wrong j (col) coord in Image");
}
