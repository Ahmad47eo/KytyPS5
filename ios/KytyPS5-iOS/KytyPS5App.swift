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
    @State private var coreDiagnostic = "Not run yet"

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

                    Text("The native iOS shell and ARM64 interpreter prototype build in CI. The small tested instruction subset is not the upstream Kyty runtime, so this app still cannot run PS5 games.")
                        .font(.body)

                    GroupBox("Porting milestones") {
                        VStack(alignment: .leading, spacing: 12) {
                            milestone("Native SwiftUI app shell", complete: true)
                            milestone("Compile iOS ARM64 core and Zydis", complete: true)
                            milestone("Test bounded x86-64 instruction subset", complete: true)
                            milestone("Connect upstream Kyty runtime", complete: false)
                            milestone("Port core and dependencies", complete: false)
                            milestone("Graphics backend for iOS", complete: false)
                            milestone("Test legal game data", complete: false)
                        }
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .padding(.top, 4)
                    }

                    GroupBox("Native core diagnostic") {
                        VStack(alignment: .leading, spacing: 10) {
                            Text("Runs a tiny built-in instruction test against the native interpreter. This is a core smoke test, not a PS5 game test.")
                                .font(.subheadline)
                            Button("Run core self-test") {
                                var resultRax: UInt64 = 0
                                var steps: Int = 0
                                let passed = kyty_ios_core_self_test(&resultRax, &steps) == 1
                                coreDiagnostic = passed
                                    ? "PASS — RAX = \(resultRax), instructions = \(steps)"
                                    : "FAIL — native core diagnostic did not match expected result"
                            }
                            .buttonStyle(.borderedProminent)
                            Text(coreDiagnostic)
                                .font(.footnote.monospaced())
                                .textSelection(.enabled)
                        }
                        .frame(maxWidth: .infinity, alignment: .leading)
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
