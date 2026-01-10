This document outlines the design and implementation details for a modular, Android-inspired application framework designed for **Zephyr RTOS**, **LVGL**, and **E-ink** hardware.

---

## 1. System Architecture Overview

The framework utilizes a **Task Stack** for navigation and a **Decentralized Registry** for application discovery. It follows a strictly event-driven, Pub-Sub model to minimize CPU wake-ups and optimize E-ink refreshes.

### Core Components

* **Activity:** A single screen (UI + Logic).
* **Application:** A logical container that owns activities and manages global app state.
* **ActivityManager:** The orchestrator that manages the navigation stack and dispatches events.
* **AppService:** The system registry that tracks "installed" applications.

---

## 2. The Application Interface

Every application must inherit from this base class. It is responsible for its own metadata and registering its own screens.

```cpp
class Application {
public:
    virtual ~Application() = default;

    // Lifecycle: Called by the system at boot
    virtual void on_install() = 0; 
    
    // Metadata for the Launcher
    virtual const char* get_name() const = 0;
    virtual const char* get_icon() const = 0;
    virtual const char* get_main_activity() const = 0;

protected:
    // Helper to link a class to a string name in the Registry
    template<typename T>
    void register_activity(const char* name) {
        ActivityRegistry::add_entry(name, []() -> Activity* { 
            return new T(); 
        });
    }
};

```

---

## 3. The Activity Interface

Activities handle the UI lifecycle. Because E-ink is a "persistent" display, the `on_create` method is where the UI is built from scratch.

```cpp
enum class Event : uint32_t {
    BtnEnter = (1 << 0), BtnBack = (1 << 1),
    BtnUp    = (1 << 2), BtnDown = (1 << 3),
    Tick     = (1 << 4), Noti     = (1 << 5)
};

class Activity {
public:
    virtual ~Activity() = default;

    // The Activity declares what hardware events it wants to hear
    virtual uint32_t event_mask() const = 0;

    // Built-in LVGL UI construction
    virtual void on_create(lv_obj_t* root, void* bundle) = 0;
    
    // Logic handler
    virtual void on_event(Event e, void* data) = 0;

    // Navigation Helpers
    void finish(); // Pops stack
    void start_activity(const char* name, void* bundle = nullptr);
};

```

---

## 4. Framework Logic (The Manager)

The `ActivityManager` maintains a `std::stack<Activity*>`.

### Navigation Flow:

1. **`start(name)`**:
* Calls `on_pause()` on the current top activity.
* Look up the "factory" for `name` in the `ActivityRegistry`.
* Calls `lv_obj_clean(lv_scr_act())` to clear the E-ink screen.
* Instantiates the new activity and calls `on_create()`.


2. **`finish()`**:
* Calls `delete` on the top activity (reclaiming RAM).
* Pops the stack.
* Re-initializes the UI of the previous activity.



### Event Dispatching:

When a physical button is pressed, Zephyr's GPIO ISR pushes an event to a `k_msgq`. The main loop then calls:

```cpp
void dispatch(Event e, void* data) {
    Activity* top = task_stack.top();
    if ((uint32_t)e & top->event_mask()) {
        top->on_event(e, data);
    }
}

```

---

## 5. Implementation Guide: Creating an App

To add a new application to the system, follow these three steps:

### Step 1: Define the Activities

```cpp
class HeartRateUI : public Activity {
public:
    uint32_t event_mask() const override { return (uint32_t)Event::BtnBack; }
    void on_create(lv_obj_t* root, void* bundle) override {
        lv_label_set_text(lv_label_create(root), "HR: 72 BPM");
    }
    void on_event(Event e, void* data) override {
        if (e == Event::BtnBack) finish();
    }
};

```

### Step 2: Define the Application Container

```cpp
class HeartRateApp : public Application {
public:
    const char* get_name() const override { return "Heart Rate"; }
    const char* get_icon() const override { return LV_SYMBOL_PULSE; }
    const char* get_main_activity() const override { return "HR_Home"; }

    void on_install() override {
        register_activity<HeartRateUI>("HR_Home");
    }
};

```

### Step 3: Register the App

Use the registration macro at the bottom of your `.cpp` file to ensure the system "discovers" the app at boot.

```cpp
REGISTER_APP(HeartRateApp);

```

---

## 6. Developer Macros

These simplify the static initialization logic required to register apps before `main()` runs.

```cpp
#define REGISTER_APP(AppClass) \
    static struct AppClass##Inst { \
        AppClass##Inst() { \
            static AppClass instance; \
            AppService::instance().register_app(&instance); \
        } \
    } global_##AppClass##Inst;

```

---

## 7. E-ink Optimization Notes

* **RAM Management:** Because only the top activity exists in memory, we can use a larger LVGL buffer for smoother drawing without running out of heap.
* **Refresh Control:** The `ActivityManager` should trigger a **Full Refresh** inside `start()` and `finish()` to clear ghosting between app transitions.
* **Partial Updates:** Inside `on_event`, developers should only update specific labels to trigger fast partial E-ink refreshes.
