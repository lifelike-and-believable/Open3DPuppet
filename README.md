# Open3DPuppet

Modular, mocap-driven virtual puppetry for Unreal Engine 5.8 and later (Win64).

Take a skeletal mesh, attach a set of puppet modules to it (biped core, limbs, digitigrade legs,
tails, wings, props), and start puppeteering it from live motion capture or recorded motion. Every
module carries its own retargeting controls, so unusual proportions and anatomy are handled where
they occur, and one puppet can be driven by several performers. Performers arrive through Live
Link, so any capture system with a Live Link source works.

Status: early development. See the [roadmap](docs/roadmap/roadmap.md).

## Documentation

- [Design overview](docs/design/overview.md): concepts, the frame pipeline, the retargeting model and the module library.
- [Authoring puppet modules](docs/guides/authoring-puppet-modules.md): how to add a module to the library.
- [Architecture decisions](docs/adr/README.md).
- [Build and CI](Build/README.md).
- [Changelog](CHANGELOG.md).

## Repository layout

| Path | Contents |
|---|---|
| `ProjectSandbox/Plugins/Open3DPuppet/` | The plugin: `Open3DPuppet` (runtime), `Open3DPuppetControlRig` (runtime, the Control Rig adapter), `Open3DPuppetPhysics` (runtime, the engine physics hand-off), `Open3DPuppetEditor`, `Open3DPuppetTests`. |
| `ProjectSandbox/` | Host project for development and tests. |
| `docs/` | Design, ADRs, roadmap and guides. |
| `Build/` | CI check scripts. |

Coding agents: start with [`AGENTS.md`](AGENTS.md).

## License

MIT, as Open3DBroadcast. See [LICENSE](LICENSE); the plugin carries the same file at
`ProjectSandbox/Plugins/Open3DPuppet/LICENSE`, and its package includes it.
