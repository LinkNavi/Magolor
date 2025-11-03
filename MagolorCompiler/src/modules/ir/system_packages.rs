// src/modules/ir/system_packages.rs
// System package registry and standard library definitions

use crate::modules::ir::ir_types::*;
use std::collections::HashMap;

pub struct SystemPackageRegistry {
    packages: HashMap<String, SystemPackage>,
}

impl SystemPackageRegistry {
    pub fn new() -> Self {
        let mut registry = Self {
            packages: HashMap::new(),
        };
        registry.register_standard_packages();
        registry
    }

    pub fn register_package(&mut self, package: SystemPackage) {
        self.packages.insert(package.name.clone(), package);
    }

    pub fn get_package(&self, name: &str) -> Option<&SystemPackage> {
        self.packages.get(name)
    }

    pub fn get_function(&self, package: &str, function: &str) -> Option<&SystemFunction> {
        self.packages
            .get(package)?
            .functions
            .iter()
            .find(|f| f.name == function)
    }

    fn register_standard_packages(&mut self) {
        self.register_package(create_console_package());
        self.register_package(create_math_package());
        self.register_package(create_string_package());
        self.register_package(create_io_package());
        self.register_package(create_memory_package());
    }
}

#[derive(Clone)]
pub struct SystemPackage {
    pub name: String,
    pub functions: Vec<SystemFunction>,
    pub types: Vec<SystemType>,
}

#[derive(Clone)]
pub struct SystemFunction {
    pub name: String,
    pub params: Vec<IRType>,
    pub return_type: IRType,
    pub is_pure: bool,
    pub is_intrinsic: bool,
    pub inline_hint: InlineHint,
}

#[derive(Clone)]
pub struct SystemType {
    pub name: String,
    pub ir_type: IRType,
}

// Console package
fn create_console_package() -> SystemPackage {
    SystemPackage {
        name: "Console".to_string(),
        functions: vec![
            SystemFunction {
                name: "print".to_string(),
                params: vec![IRType::Ptr(Box::new(IRType::I8))],
                return_type: IRType::Void,
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
            SystemFunction {
                name: "println".to_string(),
                params: vec![IRType::Ptr(Box::new(IRType::I8))],
                return_type: IRType::Void,
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
            SystemFunction {
                name: "read".to_string(),
                params: vec![],
                return_type: IRType::Ptr(Box::new(IRType::I8)),
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
            SystemFunction {
                name: "readLine".to_string(),
                params: vec![],
                return_type: IRType::Ptr(Box::new(IRType::I8)),
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
        ],
        types: vec![],
    }
}

// Math package
fn create_math_package() -> SystemPackage {
    SystemPackage {
        name: "Math".to_string(),
        functions: vec![
            SystemFunction {
                name: "sqrt".to_string(),
                params: vec![IRType::F64],
                return_type: IRType::F64,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "pow".to_string(),
                params: vec![IRType::F64, IRType::F64],
                return_type: IRType::F64,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "sin".to_string(),
                params: vec![IRType::F64],
                return_type: IRType::F64,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "cos".to_string(),
                params: vec![IRType::F64],
                return_type: IRType::F64,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "tan".to_string(),
                params: vec![IRType::F64],
                return_type: IRType::F64,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "abs".to_string(),
                params: vec![IRType::I32],
                return_type: IRType::I32,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "absf".to_string(),
                params: vec![IRType::F64],
                return_type: IRType::F64,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "min".to_string(),
                params: vec![IRType::I32, IRType::I32],
                return_type: IRType::I32,
                is_pure: true,
                is_intrinsic: false,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "max".to_string(),
                params: vec![IRType::I32, IRType::I32],
                return_type: IRType::I32,
                is_pure: true,
                is_intrinsic: false,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "floor".to_string(),
                params: vec![IRType::F64],
                return_type: IRType::F64,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "ceil".to_string(),
                params: vec![IRType::F64],
                return_type: IRType::F64,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "round".to_string(),
                params: vec![IRType::F64],
                return_type: IRType::F64,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
        ],
        types: vec![],
    }
}

// String package
fn create_string_package() -> SystemPackage {
    let str_ptr = IRType::Ptr(Box::new(IRType::I8));
    
    SystemPackage {
        name: "String".to_string(),
        functions: vec![
            SystemFunction {
                name: "length".to_string(),
                params: vec![str_ptr.clone()],
                return_type: IRType::I32,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::Hint,
            },
            SystemFunction {
                name: "concat".to_string(),
                params: vec![str_ptr.clone(), str_ptr.clone()],
                return_type: str_ptr.clone(),
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
            SystemFunction {
                name: "substring".to_string(),
                params: vec![str_ptr.clone(), IRType::I32, IRType::I32],
                return_type: str_ptr.clone(),
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
            SystemFunction {
                name: "indexOf".to_string(),
                params: vec![str_ptr.clone(), str_ptr.clone()],
                return_type: IRType::I32,
                is_pure: true,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
            SystemFunction {
                name: "toUpper".to_string(),
                params: vec![str_ptr.clone()],
                return_type: str_ptr.clone(),
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
            SystemFunction {
                name: "toLower".to_string(),
                params: vec![str_ptr.clone()],
                return_type: str_ptr.clone(),
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
        ],
        types: vec![],
    }
}

// IO package
fn create_io_package() -> SystemPackage {
    let str_ptr = IRType::Ptr(Box::new(IRType::I8));
    
    SystemPackage {
        name: "IO".to_string(),
        functions: vec![
            SystemFunction {
                name: "readFile".to_string(),
                params: vec![str_ptr.clone()],
                return_type: str_ptr.clone(),
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
            SystemFunction {
                name: "writeFile".to_string(),
                params: vec![str_ptr.clone(), str_ptr.clone()],
                return_type: IRType::Bool,
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
            SystemFunction {
                name: "fileExists".to_string(),
                params: vec![str_ptr.clone()],
                return_type: IRType::Bool,
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::None,
            },
        ],
        types: vec![],
    }
}

// Memory package
fn create_memory_package() -> SystemPackage {
    let void_ptr = IRType::Ptr(Box::new(IRType::Void));
    
    SystemPackage {
        name: "Memory".to_string(),
        functions: vec![
            SystemFunction {
                name: "allocate".to_string(),
                params: vec![IRType::I64],
                return_type: void_ptr.clone(),
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "free".to_string(),
                params: vec![void_ptr.clone()],
                return_type: IRType::Void,
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::Always,
            },
            SystemFunction {
                name: "copy".to_string(),
                params: vec![void_ptr.clone(), void_ptr.clone(), IRType::I64],
                return_type: IRType::Void,
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::Hint,
            },
            SystemFunction {
                name: "set".to_string(),
                params: vec![void_ptr.clone(), IRType::I8, IRType::I64],
                return_type: IRType::Void,
                is_pure: false,
                is_intrinsic: true,
                inline_hint: InlineHint::Hint,
            },
        ],
        types: vec![],
    }
}

// Helper to add custom packages easily
impl SystemPackageRegistry {
    pub fn add_custom_package(
        &mut self,
        name: &str,
        functions: Vec<(&str, Vec<IRType>, IRType, bool)>,
    ) {
        let package = SystemPackage {
            name: name.to_string(),
            functions: functions
                .into_iter()
                .map(|(fname, params, ret, pure)| SystemFunction {
                    name: fname.to_string(),
                    params,
                    return_type: ret,
                    is_pure: pure,
                    is_intrinsic: true,
                    inline_hint: if pure { InlineHint::Hint } else { InlineHint::None },
                })
                .collect(),
            types: vec![],
        };
        self.register_package(package);
    }
}
