// example_game.mg - C#/Rust Hybrid Game Scripting Example

i32 fn calculate_damage(i32 attack, i32 defense) {
    let i32 damage = attack - defense;
    if (damage < 0) {
        return 0;
    }
    return damage;
}

bool fn should_flee(i32 health, i32 max_health) {
    let i32 health_percent = (health * 100) / max_health;
    return health_percent < 20;
}

i32 fn enemy_decide_action(i32 health, i32 distance_to_player) {
    if (health < 30) {
        return 0;
    } elif (distance_to_player < 5) {
        return 1;
    } else {
        return 2;
    }
}

i32 fn resolve_attack(i32 attacker_power, i32 defender_armor, bool is_critical) {
    let i32 base_damage = calculate_damage(attacker_power, defender_armor);
    if (is_critical) {
        return base_damage * 2;
    }
    return base_damage;
}

i32 fn check_quest_progress(i32 enemies_killed, i32 items_collected) {
    let i32 required_kills = 10;
    let i32 required_items = 5;
    if (enemies_killed >= required_kills && items_collected >= required_items) {
        return 100;
    }
    let i32 kill_progress = (enemies_killed * 50) / required_kills;
    let i32 item_progress = (items_collected * 50) / required_items;
    return kill_progress + item_progress;
}

i32 fn calculate_loot_quality(i32 enemy_level, i32 player_luck) {
    let i32 base = enemy_level * 10;
    let i32 luck_bonus = player_luck * 5;
    let i32 total = base + luck_bonus;
    if (total > 100) {
        return 4;
    } elif (total > 75) {
        return 3;
    } elif (total > 50) {
        return 2;
    } else {
        return 1;
    }
}

i32 fn manhattan_distance(i32 x1, i32 y1, i32 x2, i32 y2) {
    let i32 dx = x2 - x1;
    let i32 dy = y2 - y1;
    if (dx < 0) { dx = -dx; }
    if (dy < 0) { dy = -dy; }
    return dx + dy;
}

bool fn can_use_ability(i32 last_use_time, i32 current_time, i32 cooldown) {
    let i32 elapsed = current_time - last_use_time;
    return elapsed >= cooldown;
}

i32 fn apply_status_effects(i32 health, bool is_poisoned, bool is_burning) {
    let i32 new_health = health;
    if (is_poisoned) { new_health = new_health - 2; }
    if (is_burning) { new_health = new_health - 5; }
    if (new_health < 0) { new_health = 0; }
    return new_health;
}

i32 fn calculate_wave_difficulty(i32 wave_number) {
    let i32 base_enemies = wave_number * 2;
    let i32 enemy_strength = wave_number * 10;
    return base_enemies + (enemy_strength / 10);
}

i32 fn fibonacci(i32 n) {
    if (n <= 1) { return n; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

i32 fn factorial(i32 n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

i32 fn sum_to_n(i32 n) {
    let i32 sum = 0;
    for (let i32 i = 1; i <= n; i = i + 1) {
        sum = sum + i;
    }
    return sum;
}

i32 fn main() {
    print_str("=== Magolor Game Scripting Test ===");
    
    print_str("\n--- Combat Test ---");
    let i32 damage = resolve_attack(50, 20, true);
    print_str("Critical damage dealt:");
    print_int(damage);
    
    print_str("\n--- AI Test ---");
    let i32 action = enemy_decide_action(25, 3);
    print_str("Enemy action (0=flee, 1=attack, 2=patrol):");
    print_int(action);
    
    print_str("\n--- Quest Test ---");
    let i32 progress = check_quest_progress(7, 4);
    print_str("Quest progress:");
    print_int(progress);
    
    print_str("\n--- Loot Test ---");
    let i32 loot = calculate_loot_quality(10, 8);
    print_str("Loot quality (1-4):");
    print_int(loot);
    
    print_str("\n--- Pathfinding Test ---");
    let i32 dist = manhattan_distance(0, 0, 10, 15);
    print_str("Manhattan distance:");
    print_int(dist);
    
    print_str("\n--- Status Effect Test ---");
    let i32 health = apply_status_effects(100, true, true);
    print_str("Health after poison + burn:");
    print_int(health);
    
    print_str("\n--- Recursion Test ---");
    let i32 fib = fibonacci(10);
    print_str("Fibonacci(10):");
    print_int(fib);
    
    let i32 fact = factorial(5);
    print_str("Factorial(5):");
    print_int(fact);
    
    print_str("\n--- Loop Test ---");
    let i32 sum = sum_to_n(100);
    print_str("Sum 1 to 100:");
    print_int(sum);
    
    print_str("\n--- Wave System Test ---");
    let i32 difficulty = calculate_wave_difficulty(5);
    print_str("Wave 5 difficulty:");
    print_int(difficulty);
    
    print_str("\n=== All Tests Complete ===");
    
    return 0;
}
