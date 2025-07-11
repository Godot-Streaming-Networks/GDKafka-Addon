# C++ Code Standards  

This document outlines the coding standards for the project. Adhering to these standards ensures consistency, readability, and maintainability of the codebase.  

---

## Variables  

- **Constants**: Must be written in all capital letters.  
    ```cpp  
    const int MAX_VALUE = 100;  
    ```  

- **Function Parameters**: Must start with `p_`.  
    ```cpp  
    void SetValue(int p_value);  
    ```  

- **Member Variables**: Must start with `m_`.  
    ```cpp  
    int m_count;  
    ```  

- **Static Variables**: Must start with `s_`.  
    ```cpp  
    static int s_instanceCount;  
    ```  

---

## Functions  

- **Private Functions**: Must use camelCasing.  
    ```cpp  
    void calculateSum();  
    ```  

- **Public Functions**: Must use PascalCasing.  
    ```cpp  
    void CalculateSum();  
    ```  

---

## Class Structure  

Classes must follow a structured order for member visibility:  

1. **Public Members**  
   - Public functions and variables must be declared at the top of the class.  

2. **Protected Members**  
   - Protected functions and variables must be declared in the middle of the class.  

3. **Private Members**  
   - Private functions and variables must be declared at the bottom of the class.  

### Example  

```cpp  
class ExampleClass  
{  
public:  
    ExampleClass();  
    void PublicFunction();  

protected:  
    void ProtectedFunction();  

private:  
    void privateFunction();  
    int m_privateVariable;  
};  
```

# RTTI (Run-Time Type Information)
* RTTI should be avoided by default.
Prefer compile-time polymorphism (e.g., templates or CRTP) or explicit virtual interfaces.

* Only use dynamic_cast or typeid when absolutely necessary, such as interacting with external APIs or systems that require type introspection.

* Prefer virtual dispatch and explicit interface design to eliminate the need for RTTI.

# Object Ownership and Lifetime
* Classes must self-contain their data.

    * Avoid external dependencies that outlive or manage internal class state.

    * A class should not retain raw pointers or references to external objects unless ownership/lifetime is guaranteed.

* Use smart pointers (std::unique_ptr, std::shared_ptr) to manage heap-allocated memory safely.

* Destructors must clean up all resources.

    * Avoid memory leaks or dangling resources.

    * Follow RAII principles: resource acquisition is initialization. [RAII Principles Documentation](https://en.cppreference.com/w/cpp/language/raii)  

* No resurrection:

    * Objects should not retain shared state or pointers that assume an instance still exists after it’s deleted.

## Anti-Pattern Example (Do NOT do this):
```cpp
// Dangerous: storing a reference that can outlive the object
class A {
public:
    void SetExternalRef(B& p_ref) { m_ref = &p_ref; }
private:
    B* m_ref = nullptr;
};
```


## Safe Pattern Example (Do this):
```cpp
class B;

class A {
public:
    explicit A(std::shared_ptr<B> p_b) : m_b(std::move(p_b)) {}
private:
    std::shared_ptr<B> m_b;
};
```

# Final Notes
* Always prefer clarity and maintainability over cleverness. If a piece of code is hard to understand, it should be refactored.
* Use comments to explain complex logic, but strive for self-documenting code where possible.
* Follow the principle of least surprise: code should behave in a way that is intuitive to other developers.

By following these coding standards, we can ensure that our codebase remains clean, maintainable, and easy to understand for all contributors. If you have any questions or suggestions regarding these standards, please feel free to discuss them inside issues.