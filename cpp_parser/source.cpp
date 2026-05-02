#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <vector>
#include <exception>
#include <windows.h>
#include <filesystem>

struct coleccion {
	long long uts;
	std::string utc_time;
	std::string artista;
	std::string artist_mbid;
	std::string album;
	std::string album_mbid;
	std::string cancion;
	std::string cancion_mbid;

	coleccion() : uts(0) {} // Inicializa uts a 0 por defecto
};

static std::string_view extraer_campo_sv(std::string_view linea, size_t& pos) { 
	// Se crea la función extraer_campo_sv que toma una línea como string_view y una posición por referencia.
	// Esta función se encarga de extraer el siguiente campo de la línea,
	// manejando correctamente los campos entre comillas y actualizando la posición para la próxima extracción.
	// Al usar string_view, evitamos copias innecesarias de cadenas, lo que mejora el rendimiento.
	if (pos >= linea.size()) return {};  // Si la posición actual supera el tamaño de la línea, se devuelve un string_view vacío.

	if (linea[pos] == '"') { // Si el campo comienza con una comilla, se busca la siguiente comilla para delimitar el campo completo.
		// Esto ocurre para cada registro, así está estructurada la información en el .csv.
		pos++; // Se avanza la posición para saltar la comilla inicial.
		size_t inicio = pos; // Se marca el inicio del campo después de la comilla.
		size_t fin = linea.find('"', pos); // Se busca la siguiente comilla que marca el final del campo. Si no se encuentra, se devuelve el resto de la línea.
		if (fin == std::string_view::npos) { // Si no se encuentra la comilla de cierre, se devuelve el resto de la línea.
			pos = linea.size(); // Se actualiza la posición para indicar que se ha llegado al final de la línea.
			return linea.substr(inicio);  // Se devuelve el campo desde el inicio hasta el final de la línea.
		}
		pos = fin + 2; // Se actualiza la posición para saltar la comilla de cierre y la coma que sigue al campo.
		return linea.substr(inicio, fin - inicio);  // Se devuelve el campo entre las comillas, excluyendo las comillas mismas.
	}

	size_t inicio = pos; // Si el campo no comienza con una comilla, se busca la siguiente coma para delimitar el campo. Si no se encuentra, se devuelve el resto de la línea.
	size_t fin = linea.find(',', pos);
	if (fin == std::string_view::npos) {
		pos = linea.size();
		return linea.substr(inicio);
	}
	pos = fin + 1;
	return linea.substr(inicio, fin - inicio);
}


// Función auxiliar para escapar comillas al crear el .csv.
static std::string escapar(std::string_view sv) { 
	std::string resultado; 
	resultado.reserve(sv.size()); // Se reserva espacio en el string resultado para evitar múltiples asignaciones de memoria al construir la cadena final.
	for (char ch : sv) {
		// Se itera sobre cada carácter del string_view. Si se encuentra una comilla, se agrega otra comilla para escapar el carácter, y luego se agrega el carácter actual al resultado.
		if (ch == '"') resultado += '"'; // doblar la comilla
		resultado += ch;
	}
	return resultado;
}



static long long parse_uts(std::string_view sv) { // Se crea la función parse_uts que toma un string_view y lo convierte a un número entero largo largo (long long).
	long long resultado = 0; 
	for (char ch : sv) { // Se itera sobre cada carácter del string_view para construir el número. Si se encuentra un carácter que no es un dígito, se lanza una excepción.
		if (ch < '0' || ch > '9') throw std::invalid_argument("no es numero");
		resultado = resultado * 10 + (ch - '0'); // Se multiplica el resultado actual por 10 y se suma el valor numérico del carácter actual para construir el número completo.
	}
	return resultado;
}



int main() {

	SetConsoleOutputCP(CP_UTF8); // Configura la consola para usar UTF-8, lo que permite mostrar caracteres especiales correctamente.

	std::ifstream archivo("data/raw/AusteigerListens.csv", std::ios::binary); // Se abre el archivo en modo binario para evitar problemas de codificación y permitir una lectura más eficiente.
	if (!archivo) return 1; // Si no se puede abrir el archivo, se devuelve un código de error.

	// Leer todo el archivo en memoria de una sola vez
	archivo.seekg(0, std::ios::end); // Se mueve el puntero al final del archivo para determinar su tamaño.
	size_t tam = archivo.tellg(); // Se obtiene el tamaño del archivo y se devuelve la posición actual del puntero, que en este caso es el tamaño total del archivo.
	archivo.seekg(0); // Se vuelve a mover el puntero al inicio del archivo para prepararse para la lectura.
	std::string buffer(tam, '\0'); // Se crea un string llamado buffer con el tamaño del archivo, inicializado con caracteres nulos. Aquí se almacenará el contenido del archivo en memoria.
	archivo.read(buffer.data(), tam); // Se lee el contenido del archivo directamente en el buffer tomando un puntero al área de memoria donde se almacenará la información y el tamaño a leer.
	archivo.close();  // Ya no se necesita el archivo, todo está en buffer

	std::vector<coleccion> datos; // Se crea un vector de coleccion para alm acenar los datos extraídos del archivo.
	datos.reserve(200000); // Se reserva espacio para 200,000 elementos para mejorar el rendimiento.

	// Saltar encabezado buscando el primer salto de línea en el buffer
	size_t pos_global = buffer.find('\n');
	if (pos_global == std::string_view::npos) return 1;  // Si el archivo no tiene saltos de línea, se devuelve un código de error.
	pos_global += 1;  // avanzar al primer carácter después del \n
	
	while (pos_global < buffer.size()) {

		size_t fin_linea = buffer.find('\n', pos_global); // Se busca el siguiente salto de línea para delimitar la línea actual. 
		if (fin_linea == std::string_view::npos) fin_linea = buffer.size(); // Si no se encuentra un salto de línea, se considera que la línea termina al final del buffer.

		std::string_view linea(buffer.data() + pos_global, fin_linea - pos_global);
		// Se crea un string_view llamado linea que apunta a la porción del buffer que corresponde a la línea actual, utilizando el puntero al inicio de la línea y su longitud.
		pos_global = fin_linea + 1;  // Se actualiza pos_global para apuntar al inicio de la siguiente línea, avanzando más allá del salto de línea.

		if (!linea.empty() && linea.back() == '\r') // Si la línea termina con un retorno de carro, se elimina para evitar problemas al extraer los campos.
			linea = linea.substr(0, linea.size() - 1);

		size_t pos = 0; // Se inicializa una variable pos a 0, que se usará para rastrear la posición dentro de la línea al extraer los campos.

		datos.emplace_back();
		// Se agrega un nuevo elemento vacío al vector datos para almacenar la información de la línea actual.
		// Esto se hace antes de extraer los campos para asegurar que haya un espacio reservado para almacenar los datos extraídos.
		coleccion& c = datos.back();
		// Se obtiene una referencia al último elemento del vector datos, que es el nuevo elemento agregado, para facilitar la asignación de los campos extraídos.

		try {
			// Se intenta convertir el primer campo extraído a un número entero largo largo (long long) para almacenarlo en el campo uts de la estructura coleccion.
			// Si la conversión falla, se captura la excepción y se maneja adecuadamente.
			
			c.uts = parse_uts(extraer_campo_sv(linea, pos)); // Se llama a la función extraer_campo_sv para obtener el primer campo de la línea.
			// Luego, se pasa ese campo a la función parse_uts para convertirlo long long y se asigna al campo uts de la estructura coleccion.
		}
		catch (const std::exception&) { // Si ocurre una excepción durante la conversión, se maneja aquí. Esto puede suceder si el campo no es un número válido.
			datos.pop_back();  // Si la conversión falla, se elimina el último elemento agregado al vector datos, ya que no se pudo extraer un valor válido para uts.
			std::cerr << "Linea invalida: " << linea << '\n'; 
			// Se imprime un mensaje de error en la consola indicando que la línea es inválida, junto con el contenido de la línea que causó el error.
			continue;
		}

		c.utc_time = extraer_campo_sv(linea, pos);
		c.artista = extraer_campo_sv(linea, pos);
		c.artist_mbid = extraer_campo_sv(linea, pos);
		c.album = extraer_campo_sv(linea, pos);
		c.album_mbid = extraer_campo_sv(linea, pos);
		c.cancion = extraer_campo_sv(linea, pos);
		c.cancion_mbid = extraer_campo_sv(linea, pos);
	}

	for (size_t i = 0; i < 5 && i < datos.size(); i++) { // Se imprime en la consola los primeros 5 registros del vector datos para verificar que se han extraído correctamente.
		std::cout << datos[i].uts << " | "
			<< datos[i].artista << " | "
			<< datos[i].cancion << " | "
			<< datos[i].album << '\n';
	}


	std::filesystem::create_directories("data/processed");
	std::ofstream salida("data/processed/Scrobbles_limpios.csv");
	// Se abre un archivo de salida para escribir los datos procesados. Si no se puede abrir el archivo, se devuelve un código de error.
	if (!salida) return 1;
	salida << "uts,artista,artist_mbid,cancion,cancion_mbid,album,album_mbid\n";

	for (const auto& c : datos) {
		salida << c.uts << ","
			<< '"' << escapar(c.artista) << "\","
			<< '"' << escapar(c.artist_mbid) << "\","
			<< '"' << escapar(c.cancion) << "\","
			<< '"' << escapar(c.cancion_mbid) << "\","
			<< '"' << escapar(c.album) << "\","
			<< '"' << escapar(c.album_mbid) << "\"\n";
	}

	salida.close();
	std::cerr << "Registros exportados: " << datos.size() << '\n'; // Se imprime en la consola el número total de registros exportados al archivo de salida para confirmar que se han procesado correctamente.

	return 0;
}

















