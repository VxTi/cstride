package com.stride.intellij.psi

import com.intellij.extapi.psi.ASTWrapperPsiElement
import com.intellij.lang.ASTNode
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiReference
import com.intellij.psi.util.PsiTreeUtil
import com.intellij.psi.util.elementType

abstract class StrideUserTypeMixin(node: ASTNode) : ASTWrapperPsiElement(node), StrideUserType {
    override fun getReference(): PsiReference? {
        return StrideUserTypeReference(this)
    }
}

class StrideUserTypeReference(element: StrideUserType) : com.intellij.psi.PsiReferenceBase<StrideUserType>(element, element.scopedIdentifier.textRangeInParent) {
    override fun resolve(): PsiElement? {
        val typeName = element.scopedIdentifier.text
        val file = element.containingFile as? StrideFile ?: return null

        // Find the matching type definition by searching top-level items and modules
        return findTypeInElement(file, typeName)
    }

    private fun findTypeInElement(element: PsiElement, typeName: String): PsiElement? {
        var child = element.firstChild
        while (child != null) {
            if (child is StrideTypeDefinition) {
                if (child.identifier.text == typeName) return child
            } else if (child is StrideModuleStatement) {
                val found = findTypeInElement(child, typeName)
                if (found != null) return found
            }
            child = child.nextSibling
        }
        return null
    }

    override fun getVariants(): Array<Any> {
        val file = element.containingFile as? StrideFile ?: return emptyArray()
        val variants = mutableListOf<String>()
        collectTypeVariants(file, variants)
        return variants.toTypedArray()
    }

    private fun collectTypeVariants(element: PsiElement, variants: MutableList<String>) {
        var child = element.firstChild
        while (child != null) {
            if (child is StrideTypeDefinition) {
                child.identifier.text?.let { variants.add(it) }
            } else if (child is StrideModuleStatement) {
                collectTypeVariants(child, variants)
            }
            child = child.nextSibling
        }
    }
}
