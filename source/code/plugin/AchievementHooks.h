#pragma once
class AchievementHooks {
public:
    static bool Validate();
    static void Install();
    static void RecordHunterKill(void* hunter);
    static void RecordPedDeath(void* ped);
    static void RecordDamage(void* ped, void* shot, void* source);
    static void RecordCrush(void* crushingObject);
};
