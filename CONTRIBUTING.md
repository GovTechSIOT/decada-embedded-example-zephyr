# Contributing to DECADA Embedded

decada-embedded-example-* are open-source real-time operating system examples to connect to the government IoT cloud. Contributions are an important part of the DECADA Embedded ecosystem, and our goal is to make it as simple as possible to become a contributor.

You can make contributions to the source code hosted on GitHub. All contributions will be bounded by an Apache License version 2.0 (Apache-2.0).

To encourage collaboration, as well as robust, consistent and maintainable code, we have built a set of guidelines for contributing.

<br>

## How to contribute

decada-embedded-example-* has a team of [maintainers](https://github.com/orgs/GovTechSIOT/teams/device-software-group). This team is responsible for helping you get your changes in, as well as controlling the overall quality and consistency of the software.

We accept contributions in the form of pull requests. Each pull request must be reviewed by at least one other developer experienced with the functionality. For contributions that span multiple functionalities, multiple reviewers may be necessary. After reviews are complete, we test the changes as part of a larger system. The testing includes but is not limited to: functional correctness, integration with other parts of the system, code style or formatting and regressions, such as code size increase or performance degredation. If any of the tests fail, more work will be needed before we accept the contribution.

All functions and methods must include a corresponding doxygen-styled API documentation header in the souce code.

<br>

## Types of contributions

1. **Bugfix**

    A bug fix is a change that fixes a specific defect in the codebase with backward compatibility. These are the highest priority because of the positive effect the change will have on users developing against the same code. A bug fix should be limited to restoring the documented or proven otherwise, originally intended behavior. Every bug fix should contain a test to verify results before and after the pull request. Bug fixes are candidates for patch releases.

    Create branch with the prefix bugfix/. For example bugfix/foo-bar

2. **Feature**

    A feature can be any change in the functionality, including adding a new feature, a new method or a function. A feature contribution contains a new API, capability or behavior. It does not break backward compatibility with existing APIs, capabilities or behaviors. New features should also come with documentation and comprehensive test coverage. Feature PRs are treated cautiously, and new features require a new minor version for the codebase. Features are candidates for feature releases.

    Create branch with the prefix feature/. For example feature/foo-bar

3. **Others**

    All other types of pull request not convered above will fall into this category. This includes pull requests for docmentations, code refactoring and updating of unit tests. Depending on the impact of the change, this branch may be queued to a lower priority.

    Create branch with the prefix others/. For example others/foo-bar

<br>

## Github Pull Request Workflow

![Github Pull Request Workflow](/docs/github_workflow.png)