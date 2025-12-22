void TypeChecker::checkProgram(Program& prog) {
    // Resolve all using declarations first
    for (const auto& usingDecl : prog.usings) {
        moduleResolver.resolveUsing(usingDecl);
    }
    
    // Check all classes
    for (auto& cls : prog.classes) {
        checkClass(cls);
    }
    
    // Check all functions
    for (auto& fn : prog.functions) {
        checkFunction(fn);
    }
}

void TypeChecker::checkMemberAccess(const MemberExpr& expr) {
    // Check if accessing a class member
    if (auto* ident = std::get_if<IdentExpr>(&expr.object->data)) {
        std::string className = ident->name;
        std::string memberName = expr.member;
        
        // Verify the member is public
        if (!moduleResolver.isPublic(className, memberName)) {
            error("Cannot access private member '" + memberName + 
                  "' of class '" + className + "'");
        }
    }
}
