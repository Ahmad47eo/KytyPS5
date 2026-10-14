import SwiftUI

@main
struct KytyPS5App: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

struct ContentView: View {
    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(alignment: .leading, spacing: 20) {
                    VStack(alignment: .leading, spacing: 8) {
                        Text("KytyPS5")
                            .font(.system(size: 36, weight: .bold, design: .rounded))
                        Text("iOS port experiment")
                            .font(.title3)
                            .foregroundStyle(.secondary)
                    }

                    Label("Prototype shell", systemImage: "hammer.fill")
                        .font(.headline)
                        .padding(.vertical, 10)
                        .padding(.horizontal, 12)
                        .background(.orange.opacity(0.15), in: Capsule())

                    Text("This is the first iPhone-side scaffold. The PS5 emulator core is not connected yet, so this app cannot run PS5 games.")
                        .font(.body)

                    GroupBox("Porting milestones") {
                        VStack(alignment: .leading, spacing: 12) {
                            milestone("Native SwiftUI app shell", complete: true)
                            milestone("Compile for iOS ARM64", complete: false)
                            milestone("Port core and dependencies", complete: false)
                            milestone("Graphics backend for iOS", complete: false)
                            milestone("Test legal game data", complete: false)
                        }
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .padding(.top, 4)
                    }

                    GroupBox("Why games do not run yet") {
                        Text("The desktop project relies on x86-64 code paths, Vulkan 1.3, desktop libraries, and execution behavior that iOS does not provide in the same way. These dependencies must be ported and tested.")
                            .frame(maxWidth: .infinity, alignment: .leading)
                    }

                    Text("No games or copyrighted console firmware are included.")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                }
                .padding(20)
            }
            .navigationTitle("Port status")
            .navigationBarTitleDisplayMode(.inline)
        }
        .preferredColorScheme(.dark)
    }

    @ViewBuilder
    private func milestone(_ title: String, complete: Bool) -> some View {
        HStack(alignment: .top, spacing: 10) {
            Image(systemName: complete ? "checkmark.circle.fill" : "circle")
                .foregroundStyle(complete ? .green : .secondary)
            Text(title)
                .font(.subheadline)
            Spacer(minLength: 0)
        }
    }
}
