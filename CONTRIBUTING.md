# Contributing

m5stack-pet is a personal, single-maintainer project built for two small
kids. It's open source in case it helps someone build their own, but it isn't
trying to grow into a general-purpose virtual pet. That shapes how
contributions are handled:

- **Bug reports and ideas are welcome** via [issues](https://github.com/mjnitz02/m5stack-pet/issues).
- **PRs may be declined** if they don't fit the design guardrails — even if
  they're well made. Please open an issue to discuss anything non-trivial
  first. Forking it and making it yours is just as good an outcome.

## Design guardrails (non-negotiable)

1. **No reading.** It's for four- and five-year-olds. Everything outside the
   parent screen is icons and pictures.
2. **Nothing dies, nothing is lost.** No scores, no way to lose, and the
   worst a neglected pet does is ask for attention.
3. **Battery first.** No radio code, and the stick turns itself off when
   nobody is playing.
4. **Offline.** No Bluetooth, no Wi-Fi, no app.

## Development

Everything runs through the `Makefile` (see [`docs/development.md`](docs/development.md) for
the full list). You need [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/),
[uv](https://docs.astral.sh/uv/) and [gitleaks](https://github.com/gitleaks/gitleaks).

```sh
make install-hooks   # pre-commit hook (ruff + art checks + secret scan)

make test            # checks on the animal art
make lint            # ruff on tools/, then a build with warnings as errors
make flash           # compile and upload over USB
```

## Before you open a PR

- `make test` and `make lint` pass. CI runs both, and builds for the
  M5StickC Plus as well as the M5StickS3.
- Try it on a stick if the change is something a kid would see or feel.
- Keep changes focused; match the style of the surrounding code.
- Fill in the PR template.

## License

By contributing, you agree your contributions are licensed under the
[MIT License](LICENSE).
