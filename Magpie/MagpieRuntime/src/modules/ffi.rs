// src/modules/ffi.rs - Foreign Function Interface for engine integration

use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use crate::modules::bytecode::{Val, Module, VM, Compiler};
use crate::modules::tokenizer;
use crate::modules::parser;

pub type NativeFn = Box<dyn Fn(&[Val]) -> Result<Val, String> + Send + Sync>;

/// Script engine that can be embedded into a game engine
pub struct ScriptEngine {
    modules: HashMap<String, Module>,
    vms: HashMap<String, VM>,
    native_fns: Arc<Mutex<HashMap<String, NativeFn>>>,
    globals: Arc<Mutex<HashMap<String, Val>>>,
}

impl ScriptEngine {
    pub fn new() -> Self {
        let mut engine = Self {
            modules: HashMap::new(),
            vms: HashMap::new(),
            native_fns: Arc::new(Mutex::new(HashMap::new())),
            globals: Arc::new(Mutex::new(HashMap::new())),
        };
        engine.register_builtins();
        engine
    }

    fn register_builtins(&mut self) {
        // Math functions
        self.register_native("sqrt", |args| {
            Ok(Val::Float(args[0].as_float()?.sqrt()))
        });
        self.register_native("abs", |args| {
            match &args[0] {
                Val::Int(n) => Ok(Val::Int(n.abs())),
                Val::Float(f) => Ok(Val::Float(f.abs())),
                _ => Err("abs requires number".into()),
            }
        });
        self.register_native("pow", |args| {
            Ok(Val::Float(args[0].as_float()?.powf(args[1].as_float()?)))
        });
        self.register_native("sin", |args| {
            Ok(Val::Float(args[0].as_float()?.sin()))
        });
        self.register_native("cos", |args| {
            Ok(Val::Float(args[0].as_float()?.cos()))
        });
        self.register_native("tan", |args| {
            Ok(Val::Float(args[0].as_float()?.tan()))
        });
        self.register_native("floor", |args| {
            Ok(Val::Int(args[0].as_float()?.floor() as i64))
        });
        self.register_native("ceil", |args| {
            Ok(Val::Int(args[0].as_float()?.ceil() as i64))
        });
        self.register_native("round", |args| {
            Ok(Val::Int(args[0].as_float()?.round() as i64))
        });
        self.register_native("min", |args| {
            let a = args[0].as_float()?;
            let b = args[1].as_float()?;
            Ok(Val::Float(a.min(b)))
        });
        self.register_native("max", |args| {
            let a = args[0].as_float()?;
            let b = args[1].as_float()?;
            Ok(Val::Float(a.max(b)))
        });
        self.register_native("clamp", |args| {
            let v = args[0].as_float()?;
            let min = args[1].as_float()?;
            let max = args[2].as_float()?;
            Ok(Val::Float(v.max(min).min(max)))
        });
        self.register_native("lerp", |args| {
            let a = args[0].as_float()?;
            let b = args[1].as_float()?;
            let t = args[2].as_float()?;
            Ok(Val::Float(a + (b - a) * t))
        });
        self.register_native("random", |_| {
            use std::time::{SystemTime, UNIX_EPOCH};
            let seed = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_nanos();
            Ok(Val::Float(((seed % 10000) as f64) / 10000.0))
        });
        self.register_native("random_range", |args| {
            use std::time::{SystemTime, UNIX_EPOCH};
            let min = args[0].as_float()?;
            let max = args[1].as_float()?;
            let seed = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_nanos();
            let t = ((seed % 10000) as f64) / 10000.0;
            Ok(Val::Float(min + (max - min) * t))
        });
    }

    /// Register a native function callable from scripts
    pub fn register_native<F>(&mut self, name: &str, func: F)
    where
        F: Fn(&[Val]) -> Result<Val, String> + Send + Sync + 'static,
    {
        self.native_fns.lock().unwrap().insert(name.to_string(), Box::new(func));
    }

    /// Set a global variable accessible from scripts
    pub fn set_global(&mut self, name: &str, value: Val) {
        self.globals.lock().unwrap().insert(name.to_string(), value);
    }

    /// Get a global variable from scripts
    pub fn get_global(&self, name: &str) -> Option<Val> {
        self.globals.lock().unwrap().get(name).cloned()
    }

    /// Compile a script from source code
    pub fn compile(&mut self, name: &str, source: &str) -> Result<(), String> {
        let tokens = tokenizer::tokenize(source);
        let ast = parser::parse(&tokens)?;
        let module = Compiler::new().compile(ast)?;
        self.modules.insert(name.to_string(), module);
        Ok(())
    }

    /// Load and compile a script from file
    pub fn load_file(&mut self, path: &str) -> Result<String, String> {
        let source = std::fs::read_to_string(path)
            .map_err(|e| format!("Failed to read file: {}", e))?;
        let name = std::path::Path::new(path)
            .file_stem()
            .and_then(|s| s.to_str())
            .unwrap_or("script")
            .to_string();
        self.compile(&name, &source)?;
        Ok(name)
    }

    /// Run a compiled script's main function
    pub fn run(&mut self, name: &str) -> Result<Val, String> {
        let module = self.modules.remove(name)
            .ok_or_else(|| format!("Script '{}' not found", name))?;
        let mut vm = VM::new(module);
        let result = vm.run()?;
        // Store VM for later function calls
        self.vms.insert(name.to_string(), vm);
        Ok(result)
    }

    /// Call a specific function in a script
    pub fn call_function(&mut self, script: &str, func: &str, args: &[Val]) -> Result<Val, String> {
        // For now, recompile and run - a more sophisticated version would cache
        if let Some(module) = self.modules.get(script) {
            if let Some(func_ref) = module.funcs.iter().find(|f| f.name == func) {
                // Would need VM modifications to call arbitrary functions
                // For now, return error
                return Err("Direct function calls not yet implemented".into());
            }
        }
        Err(format!("Function '{}' not found in script '{}'", func, script))
    }
}

/// Represents a script component that can be attached to game objects
pub struct ScriptComponent {
    pub script_name: String,
    pub enabled: bool,
    engine: Option<*mut ScriptEngine>,
}

impl ScriptComponent {
    pub fn new(script_name: &str) -> Self {
        Self {
            script_name: script_name.to_string(),
            enabled: true,
            engine: None,
        }
    }

    pub fn bind_engine(&mut self, engine: &mut ScriptEngine) {
        self.engine = Some(engine as *mut ScriptEngine);
    }
}

/// C-compatible API for integration with C# via P/Invoke
#[repr(C)]
pub struct CScriptEngine {
    inner: *mut ScriptEngine,
}

#[no_mangle]
pub extern "C" fn script_engine_new() -> *mut CScriptEngine {
    let engine = Box::new(ScriptEngine::new());
    let c_engine = Box::new(CScriptEngine {
        inner: Box::into_raw(engine),
    });
    Box::into_raw(c_engine)
}

#[no_mangle]
pub extern "C" fn script_engine_free(engine: *mut CScriptEngine) {
    if !engine.is_null() {
        unsafe {
            let c_engine = Box::from_raw(engine);
            if !c_engine.inner.is_null() {
                drop(Box::from_raw(c_engine.inner));
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn script_engine_compile(
    engine: *mut CScriptEngine,
    name: *const i8,
    source: *const i8,
) -> i32 {
    if engine.is_null() || name.is_null() || source.is_null() {
        return -1;
    }
    unsafe {
        let engine = &mut *(*engine).inner;
        let name = std::ffi::CStr::from_ptr(name).to_str().unwrap_or("");
        let source = std::ffi::CStr::from_ptr(source).to_str().unwrap_or("");
        match engine.compile(name, source) {
            Ok(_) => 0,
            Err(_) => -1,
        }
    }
}

#[no_mangle]
pub extern "C" fn script_engine_run(engine: *mut CScriptEngine, name: *const i8) -> i64 {
    if engine.is_null() || name.is_null() {
        return 0;
    }
    unsafe {
        let engine = &mut *(*engine).inner;
        let name = std::ffi::CStr::from_ptr(name).to_str().unwrap_or("");
        match engine.run(name) {
            Ok(Val::Int(n)) => n,
            Ok(Val::Float(f)) => f as i64,
            Ok(Val::Bool(b)) => if b { 1 } else { 0 },
            _ => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn script_engine_set_int(
    engine: *mut CScriptEngine,
    name: *const i8,
    value: i64,
) {
    if engine.is_null() || name.is_null() {
        return;
    }
    unsafe {
        let engine = &mut *(*engine).inner;
        let name = std::ffi::CStr::from_ptr(name).to_str().unwrap_or("");
        engine.set_global(name, Val::Int(value));
    }
}

#[no_mangle]
pub extern "C" fn script_engine_set_float(
    engine: *mut CScriptEngine,
    name: *const i8,
    value: f64,
) {
    if engine.is_null() || name.is_null() {
        return;
    }
    unsafe {
        let engine = &mut *(*engine).inner;
        let name = std::ffi::CStr::from_ptr(name).to_str().unwrap_or("");
        engine.set_global(name, Val::Float(value));
    }
}

#[no_mangle]
pub extern "C" fn script_engine_get_int(engine: *mut CScriptEngine, name: *const i8) -> i64 {
    if engine.is_null() || name.is_null() {
        return 0;
    }
    unsafe {
        let engine = &*(*engine).inner;
        let name = std::ffi::CStr::from_ptr(name).to_str().unwrap_or("");
        match engine.get_global(name) {
            Some(Val::Int(n)) => n,
            Some(Val::Float(f)) => f as i64,
            _ => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn script_engine_get_float(engine: *mut CScriptEngine, name: *const i8) -> f64 {
    if engine.is_null() || name.is_null() {
        return 0.0;
    }
    unsafe {
        let engine = &*(*engine).inner;
        let name = std::ffi::CStr::from_ptr(name).to_str().unwrap_or("");
        match engine.get_global(name) {
            Some(Val::Float(f)) => f,
            Some(Val::Int(n)) => n as f64,
            _ => 0.0,
        }
    }
}
