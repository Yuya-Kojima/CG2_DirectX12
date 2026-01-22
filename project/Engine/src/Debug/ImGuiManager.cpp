#include "Debug/ImGuiManager.h"

void ImGuiManager::Initialize([[maybe_unused]] WindowSystem *winApp,
                              [[maybe_unused]] Dx12Core *dx12Core,
                              [[maybe_unused]] SrvManager *srvManager) {

  // ポインタ
  dx12Core_ = dx12Core;
  srvHeap_ = srvManager->GetDescriptorHeap();

  // ImGuiのコンテキストを生成
  ImGui::CreateContext();

  // ImGuiのスタイルを設定
  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(winApp->GetHwnd());

  uint32_t srvIndex = srvManager->Allocate();

  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
      srvManager->GetCPUDescriptorHandle(srvIndex);

  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle =
      srvManager->GetGPUDescriptorHandle(srvIndex);

  ImGui_ImplDX12_Init(dx12Core->GetDevice(),
                      static_cast<int>(dx12Core->GetSwapChainResourceNum()),
                      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, srvHeap_, cpuHandle,
                      gpuHandle);
}

void ImGuiManager::Finalize() {

  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiManager::Begin() {
  // ImGuiフレーム開始
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}

void ImGuiManager::End() {

  // 描画前準備
  ImGui::Render();
}

void ImGuiManager::Draw() {

  ID3D12GraphicsCommandList *commandList = dx12Core_->GetCommandList();

  // デスクリプタヒープの配列をセットするコマンド
  ID3D12DescriptorHeap *ppHeaps[] = {srvHeap_};
  commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

  // 描画コマンドを発行
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
