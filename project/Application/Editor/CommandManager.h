#pragma once
#include <memory>
#include <vector>

/// <summary>
/// コマンドパターンの基底インターフェース
/// </summary>
class ICommand {
public:
  virtual ~ICommand() = default;

  // コマンドを実行する（初回およびRedo時に呼ばれる）
  virtual void Execute() = 0;

  // コマンドを取り消す（Undo時に呼ばれる）
  virtual void Undo() = 0;
};

/// <summary>
/// コマンドの履歴を管理し、Undo/Redoを制御するマネージャー
/// </summary>
class CommandManager {
private:
  std::vector<std::unique_ptr<ICommand>> undoStack_;
  std::vector<std::unique_ptr<ICommand>> redoStack_;

  CommandManager() = default;
  ~CommandManager() = default;

public:
  // シングルトンインスタンス
  static CommandManager *GetInstance() {
    static CommandManager instance;
    return &instance;
  }

  CommandManager(const CommandManager &) = delete;
  CommandManager &operator=(const CommandManager &) = delete;

  // 新しいコマンドを実行し、履歴に追加する
  void ExecuteCommand(std::unique_ptr<ICommand> command) {
    command->Execute();
    undoStack_.push_back(std::move(command));
    // 新しい操作を行った場合、Redo履歴は破棄される
    redoStack_.clear();
  }

  // 元に戻す (Ctrl + Z)
  void Undo() {
    if (undoStack_.empty())
      return;

    auto command = std::move(undoStack_.back());
    undoStack_.pop_back();

    command->Undo();

    redoStack_.push_back(std::move(command));
  }

  // やり直す (Ctrl + Y)
  void Redo() {
    if (redoStack_.empty())
      return;

    auto command = std::move(redoStack_.back());
    redoStack_.pop_back();

    command->Execute();

    undoStack_.push_back(std::move(command));
  }

  // 履歴を全てクリアする
  void Clear() {
    undoStack_.clear();
    redoStack_.clear();
  }
};
