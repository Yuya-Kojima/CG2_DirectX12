#include "GameManager.h"

GameManager* GameManager::GetInstance() {
    static GameManager instance;
    return &instance;
}
