# Guidelines for AI Usage in the ScummVM Project

This document provides guidance for AI tools and developers using AI
assistance when contributing to the ScummVM project.

## General Guidelines

 - ALL code, including AI-generated or assisted code, MUST comply with our code formatting conventions, code naming conventions, and commit guidelines.

 - Know your personal limits.
   AI assistance is a tool helping with development, not a replacement for development skills.


 - AI must be restrained to write the appropriate amount of code comments. Lengthy LLM-oriented explanations are forbidden.

## Licensing and Legal Requirements

All contributions must comply with our licensing requirements.
This means that all code must be compatible with GPLv3+.

## Developer's Responsibilities

AI agents cannot be used as an excuse for low quality. The human submitter
is responsible for:

- Reviewing all AI-generated code
- Ensuring compliance with licensing requirements
- Taking full responsibility for the contribution

## Attribution

When AI tools contribute to the project’s development, proper attribution
helps track the evolving role of AI in the development process.
Contributions should include an Assisted-by tag in the following format:

```
Assisted-by: AGENT_NAME:MODEL_VERSION [TOOL1] [TOOL2]
```

Where:

- `AGENT_NAME` is the name of the AI tool or framework
- `MODEL_VERSION` is the specific model version used
- `[TOOL1] [TOOL2]` are optional specialized analysis tools used
  (e.g., coccinelle, sparse, smatch, clang-tidy)

Basic development tools (git, gcc, make, editors) should not be listed.

Example:

```
Assisted-by: Claude:claude-3-opus coccinelle sparse [OR]
Assisted-by: Claude:claude-opus-4.6

```

## AI and Art

Usage of AI is NOT allowed for creating media assets, such as logos, game icons, etc. We have very skilled artists working on the project. We believe that art is something truly human that must never be replaced by AI.

