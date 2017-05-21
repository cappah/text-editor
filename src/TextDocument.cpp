#include "TextDocument.h"

// La idea es leer el file y guardarlo en buffer (quiero cargarlo en la memoria)
// Para esto uso std::ifstream para levantar el archivo
bool TextDocument::init(string &filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }
    std::stringstream inputStringStream;
    inputStringStream << inputFile.rdbuf();

    // TODO: Ver si con esto no arruino encoding
    this->buffer = this->toUtf32(inputStringStream.str());
    // this->buffer = inputStringStream.str();
    this->length = buffer.getSize();  // Posiblemente no sea necesario

    inputFile.close();
    this->initLinebuffer();
    return true;
}

bool TextDocument::saveFile(string &filename) {
    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    for (int i = 0; i < (int)this->buffer.getSize(); i++) {
        std::string s = this->convertSpecialChar(this->buffer[i], outputFile);
        outputFile << s;
    }

    outputFile.close();
    return true;
}

// TODO: Extraer a una clase externa
// TODO: Manejar otros caracteres
std::string TextDocument::convertSpecialChar(sf::Uint32 c, std::ofstream &outputFile) {
    switch(c) {
        case 225: return "á";
        case 233: return "é";
        case 237: return "í";
        case 243: return "ó";
        case 250: return "ú";
        case 241: return "ñ";
    }
    if (c < 128){
        return sf::String(c);
    } else {
        outputFile.close();
        std::cerr << "\nERROR: Can't save character: " << c << std::endl;
    }
    return "";
}

// TODO: Contar newlines mientras leo el archivo en el init
// TODO: Otros sistemas operativos manejan newlines de otras formas (ej \r)
bool TextDocument::initLinebuffer() {
    int lineStart = 0;
    this->lineBuffer.clear();
    this->lineBuffer.push_back(lineStart);

    int bufferSize = this->buffer.getSize();

    for (int i = 0; i < bufferSize; i++) {
        if (this->buffer[i] == '\n' || this->buffer[i] == 13) {
            lineStart = i+1;
            this->lineBuffer.push_back(lineStart);
        }
    }
    return true;
}

// Devuelve una copia de la linea que esta en mi buffer
sf::String TextDocument::getLine(int lineNumber) {

    int lastLine = this->lineBuffer.size() - 1;

    if (lineNumber < 0 || lineNumber > lastLine) {
        std::cerr << "lineNumber " << lineNumber << " is not a valid number line. "
            << "Max is: " << this->lineBuffer.size()-1 << std::endl;
        return "";
    }

    if (lineNumber == lastLine) {
        return this->buffer.substring(this->lineBuffer[lineNumber]);

    } else {
        int bufferStart = this->lineBuffer[lineNumber];
        int nextBufferStart = this->lineBuffer[lineNumber + 1];  // Final no inclusive
        int cantidad = nextBufferStart - bufferStart - 1;

        return this->buffer.substring(bufferStart, cantidad);
    }
}

sf::String TextDocument::toUtf32(const std::string& inString) {
    sf::String outString = "";
    auto iterEnd = inString.cend();

    // Decode avanza el iterador
    for (auto iter = inString.cbegin(); iter != iterEnd; ) {
        sf::Uint32 out;
        iter = sf::Utf8::decode(iter, iterEnd, out);
        outString += out;
    }

    return outString;
}

void TextDocument::addTextToPos(sf::String text, int line, int charN) {
    int textSize = text.getSize();
    int bufferInsertPos = this->getBufferPos(line, charN);
    this->buffer.insert(bufferInsertPos, text);

    int lineAmount = this->lineBuffer.size();
    for (int l = line + 1; l < lineAmount; l++) {
        this->lineBuffer[l] += textSize;
    }

    for (int i = 0; i < (int)text.getSize(); i++) {
        if (text[i] == '\n' || text[i] == 13) {  // text[i] == \n
            int newLineStart = bufferInsertPos + i + 1; // Nueva linea comienza despues del nuevo \n

            // Inserto O(#lineas) y uso busqueda binaria pues los inicios de lineas son crecientes
            this->lineBuffer.insert(
               std::lower_bound(this->lineBuffer.begin(), this->lineBuffer.end(), newLineStart),
               newLineStart
            );

        }
    }
}

// TODO: Optimizar
void TextDocument::removeTextFromPos(int amount, int lineN, int charN) {
    int bufferStartPos = this->getBufferPos(lineN, charN);
    this->buffer.erase(bufferStartPos, amount);

    // TODO: SUPER OVERKILL. Esto es O(#buffer) y podria ser O(#lineas)
    // Revisitar idea de correr los lineBuffers en amount, teniendo en cuenta la cantidad de newlines borradas
    this->initLinebuffer();
}

int TextDocument::charAmountContained(int startLineN, int startCharN, int endLineN, int endCharN) {
    return this->getBufferPos(endLineN, endCharN) - this->getBufferPos(startLineN, startCharN) + 1;
}

int TextDocument::getBufferPos(int line, int charN) {
    return this->lineBuffer[line] + charN;
}

int TextDocument::charsInLine(int line) const {
    // Si es ultima linea, no puedo compararla con inicio de siguiente pues no hay siguiente
    int bufferSize = this->lineBuffer.size();

    if (line == bufferSize - 1) {
        return this->buffer.getSize() - this->lineBuffer[this->lineBuffer.size() - 1];
    } else {
        return this->lineBuffer[line+1] - this->lineBuffer[line] - 1;
    }
}

int TextDocument::getLineCount() const {
    return (int)this->lineBuffer.size();
}