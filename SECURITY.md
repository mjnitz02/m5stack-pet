# Security Policy

## Supported versions

This is a personal open-source project. Only the latest commit on `main`
receives security fixes.

## Reporting a vulnerability

Please **do not open a public issue** for security problems.

Report privately via GitHub's
[private vulnerability reporting](https://github.com/mjnitz02/m5stack-pet/security/advisories/new),
which notifies the maintainer directly and keeps the report confidential until
a fix ships.

Expect an initial response within roughly a week. As a single-maintainer
project there is no formal SLA, but reports are taken seriously and credited
in the advisory unless you'd rather stay anonymous.

## Scope

In scope: this repository's source and its GitHub Actions workflows. The
firmware has no radio code and takes no input beyond the buttons and the
motion sensor, so most reports will concern the workflows.

Out of scope: vulnerabilities in third-party dependencies (report those
upstream), and issues that require physical access to the device or an
already compromised local machine.
