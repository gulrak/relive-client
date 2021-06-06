#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <utility>

class ResourceManager
{
public:
    class Resource
    {
    public:
        Resource(std::string name, const unsigned char* data, size_t size)
            : _name(std::move(name)), _data(data), _size(size)
        {}
        const std::string& name() const { return _name; }
        const unsigned char* data() const { return _data; }
        size_t size() const { return _size; }
        bool empty() const { return !_data || !_size; }
    private:
        std::string _name;
        const unsigned char* _data;
        size_t _size;
    };

    ResourceManager();
    void registerResources(const void* data, long size);
    bool recourceAvailable(const std::string& name) const;
    Resource resourceForName(const std::string& name) const;
    //std::shared_ptr<Fl_Image> getImage(const std::string& name) const;

    static ResourceManager& instance();

private:
    typedef std::map< std::string,const void* > ResourceMap;
    ResourceMap _resources;
};

