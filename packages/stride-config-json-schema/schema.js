import { z } from "zod";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { writeFileSync } from "node:fs";

const __dirname = dirname(fileURLToPath(import.meta.url));
const DEFAULT_FILE_EXTENSION = ".sr";

const dependencySchema = z
	.object({
		name: z.string().describe("The name of the dependency"),
		version: z.string().describe("The version of the dependency"),
		path: z
			.string()
			.describe(
				"The path of the dependency. If absent, the compiler will look in the 'dependencies' directory.",
			)
			.optional(),
	})
	.describe("Represents a dependency required by the program");

z.globalRegistry.add(dependencySchema, { id: "dependency" });

const schema = z.object({
	name: z
		.string()
		.describe("The name of the generated binary / application")
		.default("project"),
	main: z
		.string()
		.describe("Path to the main file of the program")
		.default(`main${DEFAULT_FILE_EXTENSION}`),
	buildPath: z
		.string()
		.describe("Path to the build directory")
		.default("build"),
	dependencies: z
		.array(dependencySchema)
		.optional()
		.describe("List of dependencies required by the program")
		.default([]),
	target: z
		.string()
		.describe("Target machine to compile the binary for")
		.optional(),
	mode: z
		.enum(["jit", "compile"])
		.describe("Determines which mode to use when compiling the binary")
		.optional()
		.default("jit"),
});

const schemaFileName = "schema.json";
const defaultConfigFileName = "default.json";

const schemaOutputPath = join(__dirname, schemaFileName);
const defaultConfigOutputPath = join(__dirname, defaultConfigFileName);

writeFileSync(
	schemaOutputPath,
	JSON.stringify(schema.toJSONSchema({ target: "draft-07" }), null, 2),
);

writeFileSync(
	defaultConfigOutputPath,
	JSON.stringify(schema.parse({}), null, 2),
);
