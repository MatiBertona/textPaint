#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <string>
#include <vector> // Usaremos std::vector para manejar la memoria autom�ticamente
#include <glm/glm.hpp>

class Image {
public:
	// --- Constructores y Destructor ---
	Image() = default;
	Image(const std::string &fname, bool flipY=false);
	Image(int width, int height);

	// --- REEMPLAZAR ESTA SECCIÓN ---
	Image(const Image& other); // Declaración del constructor de copia
	Image& operator=(const Image& other); // Declaración del operador de asignación
	Image(Image &&other) noexcept;
	Image &operator=(Image &&other) noexcept;
	~Image();
	// ---------------------------------

	// --- Getters ---
	// --- Getters ---
	const unsigned char *GetData() const { return m_data; }
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	int GetChannels() const { return m_channels; }
	
	// --- M�todos para P�xeles ---
	void SetRGB(int row, int col, const glm::vec3 &rgb);
	void SetRGBA(int row, int col, const glm::vec4 &rgba);
	glm::vec3 GetRGB(int row, int col) const;
	glm::vec4 GetRGBA(int row, int col) const;
	
private:
	unsigned char *m_data = nullptr;
	int m_width = -1, m_height = -1, m_channels = -1;
	void check_indexes(int i, int j) const;
};

#endif
