#ifndef ESINGLETON_H
#define ESINGLETON_H

template<typename T>
class ESingleton {
private:
    static T* instance;

protected:
    // Protected constructor to prevent direct instantiation
    ESingleton(){}
    ESingleton(const ESingleton&) = delete;
    ESingleton& operator=(const ESingleton&) = delete;

public:
    // Get the single instance of the class
    static T* getInstance() {
        if (!instance) {
            instance = new T();
        }
        return instance;
    }

    T* operator->() {return instance;}
    const T* operator->() const {return instance;}
    T& operator*() {return *instance;}
    const T& operator*() const {return *instance;}
};

template <typename T>
T* ESingleton<T>::instance;

/*
 * Singletonize the class,then can use ESingleton<class name>::getInstance
 * to get the instance poninter of the class
*/
#define SINGLETONIZE_CLASS(clsName) \
protected:\
    friend class ESingleton<clsName>; \
    clsName(const clsName&) = delete; \
    clsName& operator=(const clsName&) = delete;

#endif // ESINGLETON_H
