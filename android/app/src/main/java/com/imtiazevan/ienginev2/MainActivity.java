package com.imtiazevan.ienginev2;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends SDLActivity {
    private static final String TAG = "iEngineV2";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        copyBundledAssets();
        super.onCreate(savedInstanceState);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL3",
            "main"
        };
    }

    private void copyBundledAssets() {
        try {
            copyAssetPath("", getFilesDir());
        } catch (IOException exception) {
            Log.e(TAG, "Failed to copy bundled assets.", exception);
        }
    }

    private void copyAssetPath(String assetPath, File outputRoot) throws IOException {
        AssetManager assetManager = getAssets();
        String[] children = assetManager.list(assetPath);

        if (children == null) {
            return;
        }

        if (children.length == 0) {
            copyAssetFile(assetPath, new File(outputRoot, assetPath));
            return;
        }

        File outputDirectory = assetPath.isEmpty() ? outputRoot : new File(outputRoot, assetPath);
        if (!outputDirectory.exists() && !outputDirectory.mkdirs()) {
            throw new IOException("Failed to create asset directory: " + outputDirectory);
        }

        for (String child : children) {
            String childPath = assetPath.isEmpty() ? child : assetPath + "/" + child;
            copyAssetPath(childPath, outputRoot);
        }
    }

    private void copyAssetFile(String assetPath, File outputFile) throws IOException {
        File parent = outputFile.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            throw new IOException("Failed to create asset parent directory: " + parent);
        }

        try (
            InputStream input = getAssets().open(assetPath);
            OutputStream output = new FileOutputStream(outputFile, false)
        ) {
            byte[] buffer = new byte[8192];
            int bytesRead;
            while ((bytesRead = input.read(buffer)) != -1) {
                output.write(buffer, 0, bytesRead);
            }
        }
    }
}
