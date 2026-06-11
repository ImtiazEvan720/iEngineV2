#ifndef IENGINEV2_ASSET_H
#define IENGINEV2_ASSET_H

#include <string>

class Asset {
public:
    enum class Type {
        Unknown,
        Texture,
        Sound,
        Music,
        Level
    };

    Asset(std::string name, std::string path, Type type = Type::Unknown);
    virtual ~Asset() = default;

    Asset(const Asset& other) = delete;
    Asset& operator=(const Asset& other) = delete;
    Asset(Asset&& other) noexcept = default;
    Asset& operator=(Asset&& other) noexcept = default;

    virtual bool load() = 0;

    const std::string& getName() const;
    const std::string& getPath() const;
    Type getType() const;

    bool isLoaded() const;
    static const char* typeToString(Type type);

protected:
    void setLoaded(bool loaded);

private:
    std::string name;
    std::string path;
    Type type;
    bool loaded;
};

#endif
