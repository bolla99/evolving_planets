#ifndef PLANETSPOPULATION_HPP
#define PLANETSPOPULATION_HPP

#include <Planet.hpp>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>

struct FileHeader {
    char magic[4];      // es. "CRL\0"
    uint64_t payload;   // lunghezza payload in bytes
    uint32_t checksum;  // checksum FNV-1a sul payload
};

static uint32_t fnv1a32(const void* data, size_t len) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; ++i) { h ^= bytes[i]; h *= 16777619u; }
    return h;
}

class PlanetsPopulation {
public:
    PlanetsPopulation() = default;
    PlanetsPopulation(
                      const std::vector<std::shared_ptr<Planet>>& planets,
                      float radius,
                      int uSize, int vSize
                      );
    
    const std::vector<Planet>& planets() const;
    float radius() const;
    int uSize() const;
    int vSize() const;
    
    // SERIALIZE
    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(_planets, _uSize, _vSize, _radius);
    }
    
    // SAVE WITH HEADER
    void save(std::string path) {
        std::ostringstream os(std::ios::binary);
        cereal::BinaryOutputArchive oarchive(os);
        oarchive(*this);
        
        auto payload = os.str();
        
        // file header
        FileHeader h;
        h.magic[0] = 'P';
        h.magic[1] = 'O';
        h.magic[2] = 'P';
        h.magic[3] = 'U';
        h.payload = static_cast<uint64_t>(payload.size());
        h.checksum = fnv1a32(payload.data(), payload.size());
        
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) throw std::runtime_error("Cannot open population file to write");
        ofs.write(reinterpret_cast<const char*>(&h), sizeof(h));
        ofs.write(payload.data(), payload.size());
    }
    
    // LOAD
    static PlanetsPopulation load(std::string path) {
        
        std::cout << "population file called" << std::endl;
        
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) throw std::runtime_error("Cannot open population file to read");
        FileHeader h;
        ifs.read(reinterpret_cast<char*>(&h), sizeof(h));
        if (ifs.gcount() < sizeof(h)) throw std::runtime_error("population file too small");
        
        std::cout << "checking magic..." << std::endl;
        
        if (h.magic[0] != 'P' or h.magic[1] != 'O' or h.magic[2] != 'P' or h.magic[3] != 'U') throw std::runtime_error("not a population file");
        
        ifs.seekg(0, std::ios::end);
        auto end = ifs.tellg();
        if (sizeof(h) + h.payload != end) throw std::runtime_error("population file has size mismatch");
        
        ifs.seekg(sizeof(h));
        auto payload = std::string();
        payload.resize(h.payload);
        ifs.read(payload.data(), h.payload);
        if (ifs.gcount() != h.payload) throw std::runtime_error("failed reading payload of population file");
        
        // checksum
        if (fnv1a32(payload.data(), payload.size()) != h.checksum) throw std::runtime_error("population file checksum mismatch");
        
        auto out = PlanetsPopulation();
        cereal::BinaryInputArchive iarchive(ifs);
        
        try {
                std::istringstream iss(payload, std::ios::binary);
                cereal::BinaryInputArchive iarchive(iss);
                iarchive(out);
            } catch (const std::exception &e) {
                throw std::runtime_error(std::string("Deserialization failed: ") + e.what());
            }
        return out;
    }
    
    
private:
    std::vector<Planet> _planets;
    int _uSize, _vSize;
    float _radius;
};

#endif
